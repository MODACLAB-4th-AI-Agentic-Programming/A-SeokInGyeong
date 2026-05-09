// Copyright SEOK IN GYEONG. NPC Chat actor component implementation.
#include "NpcChatComponent.h"

#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Policies/CondensedJsonPrintPolicy.h"

UNpcChatComponent::UNpcChatComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UNpcChatComponent::SendPlayerMessage(const FString& PlayerInput)
{
    if (PlayerInput.IsEmpty())
    {
        UE_LOG(LogNpcChat, Warning, TEXT("SendPlayerMessage: empty input, ignored."));
        return;
    }

    if (bRequestInFlight)
    {
        UE_LOG(LogNpcChat, Warning, TEXT("SendPlayerMessage: previous request still in flight."));
        OnChatRequestFailed.Broadcast(TEXT("이전 요청 처리 중"));
        return;
    }

    PendingUserInput = PlayerInput;

    // 요청 바디 직렬화
    TSharedRef<FJsonObject> Body = BuildRequestBody(PlayerInput);
    FString BodyString;
    TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer
        = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&BodyString);
    FJsonSerializer::Serialize(Body, Writer);

    UE_LOG(LogNpcChat, Log, TEXT("[%s] Outgoing request body:\n%s"),
           *NpcName, *BodyString);

    // ── HTTP 송신 ──
    FHttpModule& Http = FHttpModule::Get();
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Http.CreateRequest();
    Request->SetURL(ServerUrl);
    Request->SetVerb(TEXT("POST"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    Request->SetHeader(TEXT("Connection"), TEXT("close"));   // 연결 풀 재사용 방지 — 스테일 연결 실패 해결
    Request->SetTimeout(static_cast<float>(TimeoutSeconds));
    Request->SetContentAsString(BodyString);
    Request->OnProcessRequestComplete().BindUObject(
        this, &UNpcChatComponent::HandleHttpResponse);

    if (!Request->ProcessRequest())
    {
        UE_LOG(LogNpcChat, Error, TEXT("[%s] Failed to start HTTP request."), *NpcName);
        OnChatRequestFailed.Broadcast(TEXT("HTTP 요청 시작 실패"));
        return;
    }

    bRequestInFlight = true;
    UE_LOG(LogNpcChat, Log, TEXT("[%s] HTTP request started → %s"),
           *NpcName, *ServerUrl);
}

void UNpcChatComponent::ClearHistory()
{
    History.Reset();
}

void UNpcChatComponent::SetAffinity(int32 NewAffinity)
{
    Affinity = FMath::Clamp(NewAffinity, 0, 100);
}

void UNpcChatComponent::HandleHttpResponse(FHttpRequestPtr Request,
                                           FHttpResponsePtr Response,
                                           bool bWasSuccessful)
{
    bRequestInFlight = false;

    if (!bWasSuccessful || !Response.IsValid())
    {
        if (RetryCount < MaxRetries)
        {
            RetryCount++;
            UE_LOG(LogNpcChat, Warning, TEXT("[%s] Connection failed, retrying (%d/%d)..."),
                   *NpcName, RetryCount, MaxRetries);
            SendPlayerMessage(PendingUserInput);
            return;
        }
        RetryCount = 0;
        UE_LOG(LogNpcChat, Error, TEXT("[%s] HTTP failed after %d retries."), *NpcName, MaxRetries);
        OnChatRequestFailed.Broadcast(TEXT("연결 실패 (재시도 초과)"));
        return;
    }
    RetryCount = 0;   // 성공 시 초기화

    const int32 StatusCode = Response->GetResponseCode();
    const FString RawBody = Response->GetContentAsString();

    UE_LOG(LogNpcChat, Log, TEXT("[%s] HTTP %d, raw response:\n%s"),
           *NpcName, StatusCode, *RawBody);

    if (StatusCode != 200)
    {
        OnChatRequestFailed.Broadcast(FString::Printf(TEXT("HTTP %d"), StatusCode));
        return;
    }

    // 1) Top-level 응답 파싱
    TSharedPtr<FJsonObject> Root;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(RawBody);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        UE_LOG(LogNpcChat, Error, TEXT("[%s] Top-level JSON parse failed."), *NpcName);
        OnChatRequestFailed.Broadcast(TEXT("응답 JSON 파싱 실패"));
        return;
    }

    // 2) choices[0].message.content 추출
    const TArray<TSharedPtr<FJsonValue>>* ChoicesArray = nullptr;
    if (!Root->TryGetArrayField(TEXT("choices"), ChoicesArray)
        || !ChoicesArray
        || ChoicesArray->Num() == 0)
    {
        UE_LOG(LogNpcChat, Error, TEXT("[%s] No choices in response."), *NpcName);
        OnChatRequestFailed.Broadcast(TEXT("응답에 choices 없음"));
        return;
    }

    const TSharedPtr<FJsonObject>* FirstChoiceObj = nullptr;
    if (!(*ChoicesArray)[0]->TryGetObject(FirstChoiceObj) || !FirstChoiceObj)
    {
        OnChatRequestFailed.Broadcast(TEXT("choices[0] 객체 아님"));
        return;
    }

    const TSharedPtr<FJsonObject>* MessageObj = nullptr;
    if (!(*FirstChoiceObj)->TryGetObjectField(TEXT("message"), MessageObj) || !MessageObj)
    {
        OnChatRequestFailed.Broadcast(TEXT("choices[0].message 없음"));
        return;
    }

    FString AssistantContent;
    if (!(*MessageObj)->TryGetStringField(TEXT("content"), AssistantContent))
    {
        OnChatRequestFailed.Broadcast(TEXT("message.content 없음"));
        return;
    }

    // 3) content를 다시 JSON으로 파싱 시도
    FString Reply, Emotion;
    if (!ParseAssistantJson(AssistantContent, Reply, Emotion))
    {
        // fallback — 모델이 JSON 안 지킴. content 원문을 reply로.
        UE_LOG(LogNpcChat, Warning,
            TEXT("[%s] Assistant content not in JSON format, using raw as reply."),
            *NpcName);
        Reply = AssistantContent;
        Emotion = TEXT("neutral");
    }

    UE_LOG(LogNpcChat, Log, TEXT("[%s] Parsed reply='%s', emotion='%s'"),
           *NpcName, *Reply, *Emotion);

    // History 누적
    {
        FNpcChatMessage UserEntry;
        UserEntry.Role = TEXT("user");
        UserEntry.Content = PendingUserInput;
        History.Add(UserEntry);

        FNpcChatMessage AssistantEntry;
        AssistantEntry.Role = TEXT("assistant");
        AssistantEntry.Content = Reply;
        History.Add(AssistantEntry);

        TrimHistory();
    }

    PendingUserInput.Reset();

    OnChatResponseReceived.Broadcast(Reply, Emotion, Affinity);
}

FString UNpcChatComponent::BuildSystemPrompt() const
{
    return FString::Printf(TEXT(
        "/no_think\n"
        "너는 %s.\n"
        "%s\n"
        "\n"
        "플레이어의 너에 대한 호감도: %d/100\n"
        "- 0~30: 차갑고 무뚝뚝하게, 짧게 답한다.\n"
        "- 31~60: 평범하고 정중하게 답한다.\n"
        "- 61~100: 따뜻하고 친근하게 답한다.\n"
        "\n"
        "반드시 아래 JSON 형식만 출력해라. 설명/주석/코드블록 절대 금지:\n"
        "{\"reply\":\"<한국어 대사>\",\"emotion\":\"<neutral|happy|sad|angry|annoyed|shy>\"}"),
        *NpcName,
        *PersonaDescription,
        Affinity);
}

TSharedRef<FJsonObject> UNpcChatComponent::BuildRequestBody(const FString& UserInput) const
{
    TArray<TSharedPtr<FJsonValue>> Messages;

    // [0] system 메시지
    {
        TSharedRef<FJsonObject> SystemMsg = MakeShared<FJsonObject>();
        SystemMsg->SetStringField(TEXT("role"), TEXT("system"));
        SystemMsg->SetStringField(TEXT("content"), BuildSystemPrompt());
        Messages.Add(MakeShared<FJsonValueObject>(SystemMsg));
    }

    // [1..N] history (오래된 것부터)
    for (const FNpcChatMessage& Msg : History)
    {
        TSharedRef<FJsonObject> HistMsg = MakeShared<FJsonObject>();
        HistMsg->SetStringField(TEXT("role"), Msg.Role);
        HistMsg->SetStringField(TEXT("content"), Msg.Content);
        Messages.Add(MakeShared<FJsonValueObject>(HistMsg));
    }

    // [N+1] 새로운 user 메시지
    {
        TSharedRef<FJsonObject> UserMsg = MakeShared<FJsonObject>();
        UserMsg->SetStringField(TEXT("role"), TEXT("user"));
        UserMsg->SetStringField(TEXT("content"), UserInput);
        Messages.Add(MakeShared<FJsonValueObject>(UserMsg));
    }

    TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>();
    Body->SetStringField(TEXT("model"), ModelName);
    Body->SetNumberField(TEXT("temperature"), Temperature);
    Body->SetBoolField(TEXT("stream"), false);   // SSE 스트리밍 비활성화 — Unreal HTTP가 처리 불가
    Body->SetArrayField(TEXT("messages"), Messages);
    return Body;
}

bool UNpcChatComponent::ParseAssistantJson(const FString& Content,
                                           FString& OutReply,
                                           FString& OutEmotion) const
{
    OutReply.Reset();
    OutEmotion.Reset();

    TSharedPtr<FJsonObject> Inner;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Content);
    if (!FJsonSerializer::Deserialize(Reader, Inner) || !Inner.IsValid())
    {
        return false;
    }

    if (!Inner->TryGetStringField(TEXT("reply"), OutReply))
    {
        return false;
    }
    // emotion 은 없어도 "neutral" 로 fallback 가능
    if (!Inner->TryGetStringField(TEXT("emotion"), OutEmotion))
    {
        OutEmotion = TEXT("neutral");
    }
    return true;
}

void UNpcChatComponent::TrimHistory()
{
    const int32 MaxItems = MaxHistoryTurns * 2;   // 1턴 = user+assistant
    if (History.Num() <= MaxItems)
    {
        return;
    }
    const int32 RemoveCount = History.Num() - MaxItems;
    History.RemoveAt(0, RemoveCount, EAllowShrinking::Yes);
}

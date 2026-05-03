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
        UE_LOG(LogNpcChat, Error, TEXT("[%s] HTTP failed: connection/no response."), *NpcName);
        OnChatRequestFailed.Broadcast(TEXT("연결 실패"));
        return;
    }

    const int32 StatusCode = Response->GetResponseCode();
    const FString RawBody = Response->GetContentAsString();

    UE_LOG(LogNpcChat, Log, TEXT("[%s] HTTP %d, raw response:\n%s"),
           *NpcName, StatusCode, *RawBody);

    if (StatusCode != 200)
    {
        OnChatRequestFailed.Broadcast(FString::Printf(TEXT("HTTP %d"), StatusCode));
        return;
    }

    // 파싱 + 델리게이트 발사는 Task 6 에서 추가. 여기서는 raw 로그만.
}

FString UNpcChatComponent::BuildSystemPrompt() const
{
    return FString::Printf(TEXT(
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
    return false;   // stub — Task 6에서 구현
}

void UNpcChatComponent::TrimHistory()
{
    // stub — Task 7에서 구현
}

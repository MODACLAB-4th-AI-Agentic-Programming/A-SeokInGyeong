// Copyright SEOK IN GYEONG. NPC Chat actor component.
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interfaces/IHttpRequest.h"
#include "NpcChatTypes.h"
#include "NpcChatComponent.generated.h"

class FJsonObject;

/**
 * NPC Actor에 부착해서 LM Studio와 채팅하는 컴포넌트.
 * 페르소나 / 호감도 / 대화기록을 자체 보유한다 (NPC별 격리).
 */
UCLASS(ClassGroup=(NpcChat), meta=(BlueprintSpawnableComponent),
       DisplayName="NPC Chat")
class AI_NPC_CHAT_API UNpcChatComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UNpcChatComponent();

    // ── 서버 설정 ──
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPC Chat|Server")
    FString ServerUrl = TEXT("http://localhost:1234/v1/chat/completions");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPC Chat|Server")
    FString ModelName = TEXT("qwen2.5-7b-instruct-uncensored");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPC Chat|Server",
              meta=(ClampMin="0.0", ClampMax="2.0"))
    float Temperature = 0.7f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPC Chat|Server",
              meta=(ClampMin="1"))
    int32 TimeoutSeconds = 60;

    // ── 페르소나 ──
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPC Chat|Persona")
    FString NpcName = TEXT("이름없는 NPC");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPC Chat|Persona",
              meta=(MultiLine="true"))
    FString PersonaDescription;

    // ── 호감도 ──
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPC Chat|Affinity",
              meta=(ClampMin="0", ClampMax="100"))
    int32 Affinity = 30;

    // ── 대화 기록 ──
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPC Chat|History",
              meta=(ClampMin="0", ClampMax="20"))
    int32 MaxHistoryTurns = 6;

    UPROPERTY(BlueprintReadOnly, Category="NPC Chat|History")
    TArray<FNpcChatMessage> History;

    // ── 공개 API ──
    UFUNCTION(BlueprintCallable, Category="NPC Chat")
    void SendPlayerMessage(const FString& PlayerInput);

    UFUNCTION(BlueprintCallable, Category="NPC Chat")
    void ClearHistory();

    UFUNCTION(BlueprintCallable, Category="NPC Chat")
    void SetAffinity(int32 NewAffinity);

    // ── 델리게이트 ──
    UPROPERTY(BlueprintAssignable, Category="NPC Chat")
    FOnNpcChatResponse OnChatResponseReceived;

    UPROPERTY(BlueprintAssignable, Category="NPC Chat")
    FOnNpcChatFailed OnChatRequestFailed;

private:
    /** HTTP 응답 처리. */
    void HandleHttpResponse(FHttpRequestPtr Request,
                            FHttpResponsePtr Response,
                            bool bWasSuccessful);

    /** 시스템 프롬프트 빌드 (Affinity, Persona 포함). */
    FString BuildSystemPrompt() const;

    /** 요청 바디 JSON 빌드. */
    TSharedRef<FJsonObject> BuildRequestBody(const FString& UserInput) const;

    /** assistant content 문자열에서 reply/emotion JSON 추출. 실패 시 false. */
    bool ParseAssistantJson(const FString& Content,
                            FString& OutReply,
                            FString& OutEmotion) const;

    /** History 가 MaxHistoryTurns*2 초과면 오래된 항목 제거. */
    void TrimHistory();

    bool bRequestInFlight = false;
    FString PendingUserInput;
};

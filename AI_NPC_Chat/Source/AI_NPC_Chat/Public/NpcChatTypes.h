// Copyright SEOK IN GYEONG. NPC Chat types.
#pragma once

#include "CoreMinimal.h"
#include "NpcChatTypes.generated.h"

// 진단 로그 카테고리
DECLARE_LOG_CATEGORY_EXTERN(LogNpcChat, Log, All);

/** 대화 메시지 한 통 (user 또는 assistant). */
USTRUCT(BlueprintType)
struct AI_NPC_CHAT_API FNpcChatMessage
{
    GENERATED_BODY()

    /** "user" 또는 "assistant". */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPC Chat")
    FString Role;

    /** 메시지 본문. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPC Chat")
    FString Content;
};

/** 응답 성공 델리게이트 (reply, emotion, currentAffinity). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnNpcChatResponse,
    const FString&, Reply,
    const FString&, Emotion,
    int32, CurrentAffinity);

/** 응답 실패 델리게이트 (errorMessage). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNpcChatFailed,
    const FString&, ErrorMessage);

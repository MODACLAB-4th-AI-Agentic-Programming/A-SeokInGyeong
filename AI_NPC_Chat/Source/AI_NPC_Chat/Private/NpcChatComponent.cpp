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
    UE_LOG(LogNpcChat, Warning, TEXT("SendPlayerMessage: stub — not implemented yet. Input='%s'"), *PlayerInput);
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
    // stub — Task 5에서 구현
}

FString UNpcChatComponent::BuildSystemPrompt() const
{
    return FString();   // stub — Task 4에서 구현
}

TSharedRef<FJsonObject> UNpcChatComponent::BuildRequestBody(const FString& UserInput) const
{
    return MakeShared<FJsonObject>();   // stub — Task 4에서 구현
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

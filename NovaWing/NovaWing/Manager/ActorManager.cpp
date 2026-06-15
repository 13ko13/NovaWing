#include "ActorManager.h"
#include "../Game/Actor/Actor.h"

ActorManager& ActorManager::GetInstance()
{
    static ActorManager instance;
    return instance;
}

void ActorManager::Register(std::shared_ptr<Actor> pActor)
{
    //配列に登録する
    m_pActors.push_back(pActor);
}

void ActorManager::UpdateAll()
{
    //全てのActorを更新する
    for (std::shared_ptr<Actor> actor : m_pActors)
    {
        actor->Update();
    }

    //削除の対象を探して削除する
    RemoveActor();
}

void ActorManager::DrawAll()
{
    //全てのActorを描画する
    for (std::shared_ptr<Actor> actor : m_pActors)
    {
        actor->Draw();
    }
}

void ActorManager::RemoveActor()
{
    //ActorのIsDeadがtrueになっているオブジェクトのみ解除する
    m_pActors.erase(
        std::remove_if(
            m_pActors.begin(),//最初の要素
            m_pActors.end(),//最後の要素
            [](const std::shared_ptr<Actor>& actor)
            {
                return actor->IsDead();//条件
            }),
        m_pActors.end()//削除
        );
}

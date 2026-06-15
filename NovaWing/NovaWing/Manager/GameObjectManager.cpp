#include "GameObjectManager.h"
#include "../Game/GameObjects/GameObject.h"

GameObjectManager::GameObjectManager()
{

}

GameObjectManager& GameObjectManager::GetInstance()
{
    static GameObjectManager instance;
    return instance;
}

void GameObjectManager::Register(std::shared_ptr<GameObject> pGameObject)
{
    //配列に登録する
    m_pGameObjects.push_back(pGameObject);
}

void GameObjectManager::UpdateAll()
{
    //全てのGameObjectを更新する
    for (std::shared_ptr<GameObject> gameObject : m_pGameObjects)
    {
        gameObject->Update();
    }

    //削除の対象を探して削除する
    RemoveGameObject();
}

void GameObjectManager::DrawAll()
{
    //全てのGameObjectを描画する
    for (std::shared_ptr<GameObject> gameObject : m_pGameObjects)
    {
        gameObject->Draw();
    }
}

void GameObjectManager::RemoveGameObject()
{
    //ActorのIsDeadがtrueになっているオブジェクトのみ解除する
    m_pGameObjects.erase(
        std::remove_if(
            m_pGameObjects.begin(),//最初の要素
            m_pGameObjects.end(),//最後の要素
            [](const std::shared_ptr<GameObject>& gameObject)
            {
                return gameObject->IsDead();//条件
            }),
        m_pGameObjects.end()//削除
        );
}

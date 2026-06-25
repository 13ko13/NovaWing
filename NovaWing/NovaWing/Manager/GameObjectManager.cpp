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
    //UpdateAll中にpush_backされて配列の要素数が変わってしまうため
    //保留中に入れておいて、UpdateAllの後に実際の
    //配列に追加する
    m_pHoldGameObject.push_back(pGameObject);
}

void GameObjectManager::UpdateAll()
{
    //全てのGameObjectを更新する
    for (std::shared_ptr<GameObject> gameObject : m_pGameObjects)
    {
        gameObject->Update();
    }
    //保留しておいたGameObjectの配列を
    //実際の配列に格納する
    //insertはvectorのendの位置に別のvectorのbeginからendまでの要素をまとめて入れる
    m_pGameObjects.insert(
        m_pGameObjects.end(),
        m_pHoldGameObject.begin(),
        m_pHoldGameObject.end()
    );
    //格納終了したので保留していたものをクリアする
    m_pHoldGameObject.clear();

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

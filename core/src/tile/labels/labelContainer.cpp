#include "labelContainer.h"
#include "tile/mapTile.h"

LabelContainer::LabelContainer() {}

LabelContainer::~LabelContainer() {
    m_labels.clear();
}

bool LabelContainer::addLabel(const TileID& _tileID, const std::string& _styleName, LabelTransform _transform, std::string _text, Label::Type _type) {
    auto currentBuffer = m_ftContext->getCurrentBuffer();

    if (currentBuffer) {
        auto& container = m_pendingLabels[_styleName][_tileID];

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            container.emplace_back(new Label(_transform, _text, currentBuffer, _type));
        }

        container.back()->rasterize();

        return true;
    } 

    return false;
}

void LabelContainer::removeLabels(const TileID& _tileID) {
    if (m_labels.size() > 0) {
        for (auto& styleTilepair : m_labels) {
            std::string styleName = styleTilepair.first;

            for (auto& tileLabelsPair : m_labels[styleName]) {
                const TileID& tileID = tileLabelsPair.first;
                if (tileID == _tileID) {
                    m_labels[styleName][tileID].clear();
                }
            }
        }
    }
}

const std::vector<std::shared_ptr<Label>>& LabelContainer::getPendingLabels(const std::string& _styleName, const TileID& _tileID) {
    return m_pendingLabels[_styleName][_tileID];
}

const std::vector<std::shared_ptr<Label>>& LabelContainer::getLabels(const std::string& _styleName, const TileID& _tileID) {
    return m_labels[_styleName][_tileID];
}

void LabelContainer::updateOcclusions() {

    struct AABBdata {
        std::shared_ptr<Label> m_label;
        const TileID& m_tileID;
        std::string m_styleName;
    };

    if (!m_collisionWorker) {
        std::vector<isect2d::AABB> aabbs;
        
        for (auto& styleTilepair : m_labels) {
            std::string styleName = styleTilepair.first;
            for (auto& tileLabelsPair : m_pendingLabels[styleName]) {
                auto& labels = tileLabelsPair.second;
                for(auto& label : labels) {
                    if (label->isOutOfScreen() || label->getType() == Label::Type::DEBUG) {
                        continue;
                    }

                    AABBdata* data = new AABBdata { label, tileLabelsPair.first, styleName };
                    
                    isect2d::AABB aabb = label->getAABB();
                    aabb.m_userData = (void*) data;
                    aabbs.push_back(aabb);
                }
            }
        }
        
        if (aabbs.size() > 0) {
            m_collisionWorker = std::unique_ptr<LabelWorker>(new LabelWorker());
            m_collisionWorker->start(aabbs);
        }
        
    } else {
        
        if (m_collisionWorker->isReady()) {

            std::set<std::pair<AABBdata*, AABBdata*>> occlusions;
            auto aabbs = m_collisionWorker->getAABBs();
            auto pairs = m_collisionWorker->getResult();
            
            for (auto pair : pairs) {
                const auto& aabb1 = (*aabbs)[pair.first];
                const auto& aabb2 = (*aabbs)[pair.second];
                
                auto data1 = static_cast<AABBdata*>(aabb1.m_userData);
                auto data2 = static_cast<AABBdata*>(aabb2.m_userData);

                // narrow phase
                if (intersect(data1->m_label->getOBB(), data2->m_label->getOBB())) {
                    occlusions.insert({ data1, data2 });
                }
            }
            
            // no priorities, only occlude one of the two occluded label
            for (auto& pair : occlusions) {
                // TODO : occlude only if one of the two is occluded
                auto& aabbData = pair.first;
                auto& container = m_pendingLabels[aabbData->m_styleName][aabbData->m_tileID];
                auto it = std::find(container.begin(), container.end(), aabbData->m_label);
                
                {
                    std::lock_guard<std::mutex> lock(m_mutex);
                    m_pendingLabels[aabbData->m_styleName][aabbData->m_tileID].erase(it);
                }
            }

            // move pending labels to appropriate list
            for (auto& styleTilepair : m_labels) {
                std::string styleName = styleTilepair.first;

                for (auto& tileLabelsPair : m_pendingLabels[styleName]) {
                    auto& labels = m_labels[styleName][tileLabelsPair.first];
                    auto& pendingLabels = tileLabelsPair.second;
                    labels.insert(labels.end(), pendingLabels.begin(), pendingLabels.end());
                }
            }

            // clear processed labels
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_pendingLabels.clear();
            }
            
            m_collisionWorker.reset();
        }
    }
}

void LabelWorker::start(const std::vector<isect2d::AABB> _aabbbs) {
    m_aabbs = std_patch::make_unique<std::vector<isect2d::AABB>>(_aabbbs);
    
    m_pairs = std::async(std::launch::async, [&] {
        
        // broad phase collision detection
        auto pairs = intersect(*m_aabbs);
        m_finished = true;
        return std::move(pairs);
        
    });
    
}

std::set<std::pair<int, int>> LabelWorker::getResult() {
    
    if (m_pairs.valid()) {
        return std::move(m_pairs.get());
    } else {
        return std::set<std::pair<int, int>>();
    }
    
}

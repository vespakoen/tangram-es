#include "labelContainer.h"
#include "tile/mapTile.h"

#include <iostream>
#include <chrono>
#include <thread>

LabelContainer::LabelContainer() {}

LabelContainer::~LabelContainer() {
    m_labels.clear();
    m_pendingLabels.clear();
    m_collisionWorker.reset();
}

bool LabelContainer::addLabel(const TileID& _tileID, const std::string& _styleName, LabelTransform _transform, std::string _text, Label::Type _type, const glm::mat4& _model) {
    auto currentBuffer = m_ftContext->getCurrentBuffer();

    if (currentBuffer) {
        auto& container = m_pendingLabels[_styleName][_tileID];

        // lock concurrent collection
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            container.emplace_back(_transform, _text, currentBuffer, _type);
            
            container.back().rasterize();
            
            // ensure label is updated once
            container.back().update(m_viewProjection * _model, m_screenSize, 0);
        }

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
                    
                    {
                        std::lock_guard<std::mutex> lock(m_mutex);
                        m_pendingLabels[styleName][tileID].clear();
                    }
                }
            }
        }
    }
}

const std::vector<std::shared_ptr<Label>>& LabelContainer::getLabels(const std::string& _styleName, const TileID& _tileID) {
    return m_labels[_styleName][_tileID];
}

void LabelContainer::updateOcclusions() {

    if (!m_collisionWorker) {
        
        // merge pending labels from threads
        for (auto& styleTilepair : m_pendingLabels) {
            std::string styleName = styleTilepair.first;
            
            for (auto& tileLabelsPair : m_pendingLabels[styleName]) {
                const TileID& tileID = tileLabelsPair.first;
                auto& pendingLabels = m_pendingLabels[styleName][tileID];
                auto& labels = m_labels[styleName][tileID];
                
                for (auto& label : pendingLabels) {
                    labels.emplace_back(new Label(label));
                }
                
                {
                    std::lock_guard<std::mutex> lock(m_mutex);
                    pendingLabels.clear();
                }
            }
        }

        std::vector<isect2d::AABB> aabbs;
        
        for (auto& styleTilepair : m_labels) {
            std::string styleName = styleTilepair.first;
            
            for (auto& tileLabelsPair : m_labels[styleName]) {
                auto& labels = tileLabelsPair.second;
                
                for (auto& label : labels) {
                    if (!label->isVisible() || label->getType() == Label::Type::DEBUG) {
                        continue;
                    }
                    
                    isect2d::AABB aabb = label->getAABB();
                    
                    // make sure pointer is retained
                    aabb.m_userData = (void*) label.get();
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

            std::set<std::pair<Label*, Label*>> occlusions;
            auto aabbs = m_collisionWorker->getAABBs();
            auto pairs = m_collisionWorker->getResult();
            
            for (auto pair : pairs) {
                const auto& aabb1 = (*aabbs)[pair.first];
                const auto& aabb2 = (*aabbs)[pair.second];
                
                auto l1 = static_cast<Label*>(aabb1.m_userData);
                auto l2 = static_cast<Label*>(aabb2.m_userData);
                
                if (!l1 || !l2) {
                    continue;
                }
                
                // narrow phase
                if (intersect(l1->getOBB(), l2->getOBB())) {
                    occlusions.insert({ l1, l2 });
                }
            }
            
            // no priorities, only occlude one of the two occluded label
            for (auto& pair : occlusions) {
                if (pair.second->isVisible()) {
                    pair.first->setVisible(false);
                }
            }
            
            // indicate that occlusions have been solved
            for (auto& styleTilepair : m_labels) {
                std::string styleName = styleTilepair.first;
                
                for (auto& tileLabelsPair : m_labels[styleName]) {
                    auto& labels = tileLabelsPair.second;
                    
                    for (auto& label : labels) {
                        label->setOcclusionSolved();
                    }
                }
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
        
        requestRender();
        
        return std::move(pairs);
        
    });
    
}

std::unique_ptr<std::vector<isect2d::AABB>> LabelWorker::getAABBs() {
    return std::move(m_aabbs);
}

std::set<std::pair<int, int>> LabelWorker::getResult() {
    
    if (m_pairs.valid()) {
        return std::move(m_pairs.get());
    } else {
        return std::set<std::pair<int, int>>();
    }
    
}

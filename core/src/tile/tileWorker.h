#pragma once

#include <memory>
#include <future>

#include "style/style.h"

class TileManager;


class TileWorker {
    
public:
    
    TileWorker(TileManager& _tileManager);
    
    void process(const StyleSet& _styles);
    
    void abort();
    
    bool isRunning() const { return m_running; }

    void drain();
    
private:
    
    TileManager& m_tileManager;
    
    bool m_aborted;
    bool m_running;
    
    std::future<bool> m_future;
};


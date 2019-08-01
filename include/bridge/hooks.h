#pragma once

struct bridge_hooks {
    void* (*onUnload)(void);
};

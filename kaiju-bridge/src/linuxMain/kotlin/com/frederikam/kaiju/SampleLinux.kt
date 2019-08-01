package com.frederikam.kaiju

import kaiju_core.bridge_hooks
import kotlinx.cinterop.*

@Suppress("unused")
fun kaijuEntry(): bridge_hooks {
    println("Hello from Kaiju-Bridge")
    memScoped {
        val hooks = alloc<bridge_hooks>()
        hooks.onUnload = staticCFunction(::onUnload0)
        return hooks
    }
}

fun onUnload0(): COpaquePointer? {
    println("Unloaded")
    return null
}

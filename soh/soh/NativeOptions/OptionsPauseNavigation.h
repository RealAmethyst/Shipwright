#pragma once

enum NativePauseRoute { NATIVE_PAUSE_ROTATE, NATIVE_PAUSE_ENTER_OPTIONS, NATIVE_PAUSE_RETURN };

static inline enum NativePauseRoute NativeOptions_PauseRoute(int page, int right, int options,
                                                            int firstPage, int lastPage) {
    if (options) {
        return (page == lastPage && !right) || (page == firstPage && right)
            ? NATIVE_PAUSE_RETURN : NATIVE_PAUSE_ROTATE;
    }
    return (page == lastPage && right) || (page == firstPage && !right)
        ? NATIVE_PAUSE_ENTER_OPTIONS : NATIVE_PAUSE_ROTATE;
}

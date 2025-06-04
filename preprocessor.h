#pragma once

#define PASTE_IMPL(x, y) x##y

#define PASTE(x, y) PASTE_IMPL(x, y)

#pragma once
#include "vocab_loader.h"

// v2.0: Word data now loaded dynamically from /manifest.json
// WORD_LIST and WORD_COUNT are compatibility shims for card_manager
#define WORD_LIST  (vocabLoader::words())
#define WORD_COUNT (vocabLoader::wordCount())

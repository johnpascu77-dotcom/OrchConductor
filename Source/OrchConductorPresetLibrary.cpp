#include "OrchConductorPresetLibrary.h"

namespace orchconductor
{

// Phase 3E intentionally contains only passive data-model definitions.
//
// Runtime preset selection, MIDI CC generation, UI state, and hardcoded
// factory preset behavior remain implemented by the existing processor/editor
// code paths.
//
// Future phases may add JSON parsing and factory-library construction here,
// but Phase 3E deliberately avoids runtime behavior changes.

} // namespace orchconductor

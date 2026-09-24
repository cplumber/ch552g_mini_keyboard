#pragma once

// The six-key mapper uses the same candidate data pin as the original board
// family. The count is deliberately six for discovery; a dark final pixel is
// evidence that this particular strip has fewer physical LEDs.
#define PIN_NEO P34
#define NEO_COUNT 6
#define NEO_GRB

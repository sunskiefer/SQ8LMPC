// The one Sprite member the headless display needs. The upstream Sprite.cpp also decodes the editor's bitmaps,
// which the Force build does not carry (the panel is drawn by the MPC's own skin renderer).
#include "Sprite.h"
#include <utility>
namespace sq8l::gui {
Sprite::Sprite(std::string name, int frameWidth, int frameHeight, int count, std::vector<Color> pixels)
    : name_(std::move(name)), fw_(frameWidth), fh_(frameHeight), count_(count), px_(std::move(pixels)) {}
}  // namespace sq8l::gui

#include "game.h"

int main() {
    auto g = Game();
    g.load_level(1);
    g.run_with_manual_control();
    
    return 0;
}
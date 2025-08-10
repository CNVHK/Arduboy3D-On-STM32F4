#include "Game.h"
#include "Game_CApi.h"

extern "C" 
{
    void Game_Init()        { Game::Init(); }
    void Game_Tick()        { Game::Tick(); }
    void Game_Draw()        { Game::Draw(); }
    void Game_StartGame()   { Game::StartGame(); }
    void Game_NextLevel()   { Game::NextLevel(); }
    void Game_GameOver()    { Game::GameOver(); }
    void Game_ShowMessage(const char* msg) { Game::ShowMessage(msg); }

    void* Game_GetPlayer()  { return &Game::player; }

    uint8_t Game_GetFloor() { return Game::floor; }
    void    Game_SetFloor(uint8_t f) { Game::floor = f; }

    void* Game_GetStats()   { return &Game::stats; }
}
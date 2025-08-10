#ifndef __GAME_CAPI_H__
#define __GAME_CAPI_H__

#ifdef __cplusplus
extern "C" {
#endif

void Game_Init(void);
void Game_Tick(void);
void Game_Draw(void);
void Game_StartGame(void);
void Game_StartLevel(void);
void Game_NextLevel(void);
void Game_GameOver(void);
void Game_ShowMessage(const char* msg);

void*  Game_GetPlayer(void);

uint8_t Game_GetFloor(void);
void    Game_SetFloor(uint8_t f);
void*  Game_GetStats(void);

#ifdef __cplusplus
}
#endif

#endif
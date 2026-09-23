#pragma once
#ifdef __cplusplus
extern "C" {
#endif
struct GameState;
struct GraphicsContext;
struct PlayState;
void Navigation_UpdateInput(struct GameState* state);
void Navigation_Draw(struct GraphicsContext* gfxCtx);
void Navigation_CaptureCollision(struct PlayState* play);
void Navigation_PublishAudio(struct PlayState* play);
int Navigation_IsOpen(void);
int Navigation_HasRoute(void);
#ifdef __cplusplus
}
#endif

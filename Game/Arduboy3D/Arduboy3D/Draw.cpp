#include <stdint.h>
#include "Draw.h"
#include "Defines.h"
#include "Game.h"
#include "Particle.h"
#include "FixedMath.h"
#include "Map.h"
#include "Projectile.h"
#include "Platform.h"
#include "Enemy.h"
#include "Font.h"
#include "Platform.h"

#include "LUT.h"
#include "Generated/SpriteData.inc.h"
#include "oled.h"

#if WITH_VECTOR_TEXTURES
#include "Textures.h"
#endif

#if WITH_SPRITE_OUTLINES
#define DrawScaledInner DrawScaledOutline
#else
#define DrawScaledInner DrawScaledNoOutline
#endif

Camera Renderer::camera;
uint8_t Renderer::wBuffer[DISPLAY_WIDTH];
int8_t Renderer::horizonBuffer[DISPLAY_WIDTH];
uint8_t Renderer::globalRenderFrame = 0;
uint8_t Renderer::numBufferSlicesFilled = 0;
QueuedDrawable Renderer::queuedDrawables[MAX_QUEUED_DRAWABLES];
uint8_t Renderer::numQueuedDrawables = 0;

const uint8_t scaleDrawWriteMasks[] =
{
	(1),
	(1 << 1),
	(1 << 2),
	(1 << 3),
	(1 << 4),
	(1 << 5),
	(1 << 6),
	(1 << 7)
};

const uint16_t scaleDrawReadMasks[] =
{
	(1),
	(1 << 1),
	(1 << 2),
	(1 << 3),
	(1 << 4),
	(1 << 5),
	(1 << 6),
	(1 << 7),
	(1 << 8),
	(1 << 9),
	(1 << 10),
	(1 << 11),
	(1 << 12),
	(1 << 13),
	(1 << 14),
	(1 << 15)
};

#if WITH_VECTOR_TEXTURES
void Renderer::DrawWallLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t clipLeft, uint8_t clipRight, uint8_t col)
{
	if(x1 > x2)
		return;
	
	if (y1 < 0)
	{
		if(y2 < 0)
			return;
		
		if(y2 != y1)
			x1 += (0 - y1) * (x2 - x1) / (y2 - y1);
		y1 = 0;
	}
	if (y2 > DISPLAY_HEIGHT - 1)
	{
		if(y1 > DISPLAY_HEIGHT - 1)
			return;
		
		if(y2 != y1)
			x2 += (((DISPLAY_HEIGHT - 1) - y2) * (x1 - x2)) / (y1 - y2);
		y2 = DISPLAY_HEIGHT - 1;
	}
	
	if (x1 < clipLeft)
	{
		if(x2 != x1)
		{
			y1 += ((clipLeft - x1) * (y2 - y1)) / (x2 - x1);
		}
		x1 = clipLeft;
	}

	int16_t dx = x2 - x1;
	int16_t yerror = dx / 2;
	int16_t y = y1;
	int16_t dy;
	int8_t ystep;

	if (y1 < y2)
	{
		dy = y2 - y1;
		ystep = 1;
	}
	else
	{
		dy = y1 - y2;
		ystep = -1;
	}

	for (int x = x1; x <= x2 && x <= clipRight; x++)
	{
		int8_t horizon = horizonBuffer[x] - HORIZON;		

		Platform::PutPixel(x, horizon + y, col);

		yerror -= dy;

		while (yerror < 0)
		{
			y += ystep;
			
			//if(y < 0 || y >= DISPLAY_HEIGHT)
			//	return;
			
			yerror += dx;

			if (yerror < 0)
			{
				Platform::PutPixel(x, horizon + y, col);
			}

			if (x == x2 && y == y2)
				break;
		}
	}
}
#endif

#if WITH_IMAGE_TEXTURES
void Renderer::DrawWallSegment(const uint16_t* texture, int16_t x1, int16_t w1, int16_t x2, int16_t w2, uint8_t u1clip, uint8_t u2clip, bool edgeLeft, bool edgeRight, bool shadeEdge)
#elif WITH_VECTOR_TEXTURES
void Renderer::DrawWallSegment(const uint8_t* texture, int16_t x1, int16_t w1, int16_t x2, int16_t w2, uint8_t u1clip, uint8_t u2clip, bool edgeLeft, bool edgeRight, bool shadeEdge)
#else
void Renderer::DrawWallSegment(int16_t x1, int16_t w1, int16_t x2, int16_t w2, bool edgeLeft, bool edgeRight, bool shadeEdge)
#endif
{
	if (x1 < 0)
	{
#if WITH_TEXTURES
		u1clip += ((int32_t)(0 - x1) * (int32_t)(u2clip - u1clip)) / (x2 - x1);
#endif
		w1 += ((int32_t)(0 - x1) * (int32_t)(w2 - w1)) / (x2 - x1);
		x1 = 0;
		edgeLeft = false;
	}

	int16_t dx = x2 - x1;
	int16_t werror = dx / 2;
	int16_t w = w1;
	int16_t dw;
	int8_t wstep;
#if WITH_IMAGE_TEXTURES
	uint8_t du = u2clip - u1clip;
	int16_t uerror = werror;
	uint8_t u = u1clip;
#endif

	if (w1 < w2)
	{
		dw = w2 - w1;
		wstep = 1;
	}
	else
	{
		dw = w1 - w2;
		wstep = -1;
	}

	constexpr uint8_t wallColour = COLOUR_WHITE;
	constexpr uint8_t edgeColour = COLOUR_BLACK;
	
	uint8_t segmentClipLeft = (uint8_t) x1;
	uint8_t segmentClipRight = x2 < DISPLAY_WIDTH ? (uint8_t) x2 : DISPLAY_WIDTH - 1;

	for (int x = x1; x < DISPLAY_WIDTH; x++)
	{
		bool drawSlice = x >= 0 && wBuffer[x] < w;
		bool shadeSlice = shadeEdge && (x & 1) == 0;		
		
		int8_t horizon = horizonBuffer[x];

		if (drawSlice)
		{
			uint8_t sliceMask = 0xff;

			if ((edgeLeft && x == x1) || (edgeRight && x == x2))
			{
				sliceMask = 0x00;
			}
			else if (shadeSlice)
			{
				sliceMask = 0x55;
			}

#if WITH_IMAGE_TEXTURES
			{
				uint8_t y1 = w > horizon ? 0 : horizon - w;
				uint8_t y2 = horizon + w > DISPLAY_HEIGHT ? DISPLAY_HEIGHT : horizon + w;

				DrawVLine(x, y1, y2, sliceMask);
				uint16_t textureData = pgm_read_word(&texture[u % 16]);
				const uint16_t wallSize = w * 2;
				uint16_t wallPos = y1 - (horizon - w);
				
				for (uint8_t y = y1; y < y2; y++)
				{
					uint8_t v = (16 * wallPos) / wallSize;
					uint16_t mask = pgm_read_word(&scaleDrawReadMasks[v]);

					if ((textureData & mask) == 0)
					{
						Platform::PutPixel(x, y, 0);
					}

					wallPos++;
				}
			}
#else
			int8_t extent = w > 64 ? 64 : w;
			Platform::DrawVLine(x, horizon - extent, horizon + extent, sliceMask);
			Platform::PutPixel(x, horizon + extent, edgeColour);
			Platform::PutPixel(x, horizon - extent, edgeColour);
#endif
			
			if(wBuffer[x] == 0)
			{
				numBufferSlicesFilled++;
			}
			
			if (w > 255)
				wBuffer[x] = 255;
			else
				wBuffer[x] = (uint8_t)w;
		}
		else
		{
			if(x == segmentClipLeft)
			{
				segmentClipLeft++;
			}
			else if(x < segmentClipRight)
			{
				segmentClipRight = x;
				break;
			}
		}

		if (x == x2)
			break;

		werror -= dw;

		while (werror < 0)
		{
			w += wstep;
			werror += dx;

			if (drawSlice && werror < 0 && w <= DISPLAY_HEIGHT / 2)
			{
				Platform::PutPixel(x, horizon + w - 1, edgeColour);
				Platform::PutPixel(x, horizon - w, edgeColour);
			}
		}
		
		#if WITH_IMAGE_TEXTURES
		uerror -= du;
		
		while(uerror < 0)
		{
			u++;
			uerror += dx;
		}
		#endif
	}
	
	if(segmentClipLeft == segmentClipRight)
		return;

#if WITH_VECTOR_TEXTURES
	if (w1 < MIN_TEXTURE_DISTANCE || w2 < MIN_TEXTURE_DISTANCE || !texture)
		return;
	if(u1clip == u2clip)
		return;

	const uint8_t* texPtr = texture;
	uint8_t numLines = *(texPtr++);
	while (numLines)
	{
		numLines--;

		uint8_t u1 = *(texPtr++);
		uint8_t v1 = *(texPtr++);
		uint8_t u2 = *(texPtr++);
		uint8_t v2 = *(texPtr++);

		//if(u1clip != 0 || u2clip != 128)
		//	continue;
		
		if (u2 < u1clip || u1 > u2clip)
			continue;

		if (u1 < u1clip)
		{
			if(u2 != u1)
				v1 += (u1clip - u1) * (v2 - v1) / (u2 - u1);
			u1 = u1clip;
		}
		if (u2 > u2clip)
		{
			if (u2 != u1)
				v2 += (u2clip - u2) * (v1 - v2) / (u1 - u2);
			u2 = u2clip;
		}
		
		u1 = (128 * (u1 - u1clip)) / (u2clip - u1clip);
		u2 = (128 * (u2 - u1clip)) / (u2clip - u1clip);

		int16_t outU1 = (((int32_t)u1 * dx) >> 7) + x1;
		int16_t outU2 = (((int32_t)u2 * dx) >> 7) + x1;

		int16_t interpw1 = ((u1 * (w2 - w1)) >> 7) + w1;
		int16_t interpw2 = ((u2 * (w2 - w1)) >> 7) + w1;

		int16_t outV1 = (interpw1 * v1) >> 6;
		int16_t outV2 = (interpw2 * v2) >> 6;

		//uint8_t horizon = horizonBuffer[x]
		//DrawLine(ScreenSurface, outU1, HORIZON - interpw1 + outV1, outU2, HORIZON - interpw2 + outV2, edgeColour, edgeColour, edgeColour);
		DrawWallLine(outU1, HORIZON - interpw1 + outV1, outU2, HORIZON - interpw2 + outV2, segmentClipLeft, segmentClipRight, edgeColour);
		//DrawWallLine(outU1, -interpw1 + outV1, outU2, -interpw2 + outV2, edgeColour);
	}
#endif
}

bool Renderer::isFrustrumClipped(int16_t x, int16_t y)
{
	if ((camera.clipCos * (x - camera.cellX) - camera.clipSin * (y - camera.cellY)) < -512)
		return true;
	if ((camera.clipSin * (x - camera.cellX) + camera.clipCos * (y - camera.cellY)) < -512)
		return true;

	return false;
}

void Renderer::TransformToViewSpace(int16_t x, int16_t y, int16_t& outX, int16_t& outY)
{
	int32_t relX = x - camera.x;
	int32_t relY = y - camera.y;
	outY = (int16_t)((camera.rotCos * relX) >> 8) - (int16_t)((camera.rotSin * relY) >> 8);
	outX = (int16_t)((camera.rotSin * relX) >> 8) + (int16_t)((camera.rotCos * relY) >> 8);
}

void Renderer::TransformToScreenSpace(int16_t viewX, int16_t viewZ, int16_t& outX, int16_t& outW)
{
	// apply perspective projection
	outX = (int16_t)((int32_t)viewX * NEAR_PLANE * CAMERA_SCALE / viewZ);
	outW = (int16_t)((CELL_SIZE / 2 * NEAR_PLANE * CAMERA_SCALE) / viewZ);

	// transform into screen space
	outX = (int16_t)((DISPLAY_WIDTH / 2) + outX);
}

#if WITH_IMAGE_TEXTURES
void Renderer::DrawWall(const uint16_t* texture, int16_t x1, int16_t y1, int16_t x2, int16_t y2, bool edgeLeft, bool edgeRight, bool shadeEdge)
#elif WITH_VECTOR_TEXTURES
void Renderer::DrawWall(const uint8_t* texture, int16_t x1, int16_t y1, int16_t x2, int16_t y2, bool edgeLeft, bool edgeRight, bool shadeEdge)
#else
void Renderer::DrawWall(int16_t x1, int16_t y1, int16_t x2, int16_t y2, bool edgeLeft, bool edgeRight, bool shadeEdge)
#endif
{
	int16_t viewX1, viewZ1, viewX2, viewZ2;
#if WITH_VECTOR_TEXTURES
	uint8_t u1 = 0, u2 = 128;
#elif WITH_IMAGE_TEXTURES
	uint8_t u1 = 0, u2 = 16;
#endif

	TransformToViewSpace(x1, y1, viewX1, viewZ1);
	TransformToViewSpace(x2, y2, viewX2, viewZ2);

	// Frustum cull
	if (viewX2 < 0 && -2 * viewZ2 > viewX2)
		return;
	if (viewX1 > 0 && 2 * viewZ1 < viewX1)
		return;

	// clip to the front pane
	if ((viewZ1 < CLIP_PLANE) && (viewZ2 < CLIP_PLANE))
		return;

	if (viewZ1 < CLIP_PLANE)
	{
#if WITH_TEXTURES
		u1 += (CLIP_PLANE - viewZ1) * (u2 - u1) / (viewZ2 - viewZ1);
#endif
		viewX1 += (CLIP_PLANE - viewZ1) * (viewX2 - viewX1) / (viewZ2 - viewZ1);
		viewZ1 = CLIP_PLANE;
		edgeLeft = false;
	}
	else if (viewZ2 < CLIP_PLANE)
	{
#if WITH_TEXTURES
		u2 += (CLIP_PLANE - viewZ2) * (u1 - u2) / (viewZ1 - viewZ2);
#endif
		viewX2 += (CLIP_PLANE - viewZ2) * (viewX1 - viewX2) / (viewZ1 - viewZ2);
		viewZ2 = CLIP_PLANE;
		edgeRight = false;
	}

	// apply perspective projection
	int16_t vx1 = (int16_t)((int32_t)viewX1 * NEAR_PLANE * CAMERA_SCALE / viewZ1);
	int16_t vx2 = (int16_t)((int32_t)viewX2 * NEAR_PLANE * CAMERA_SCALE / viewZ2);

	// transform the end points into screen space
	int16_t sx1 = (int16_t)((DISPLAY_WIDTH / 2) + vx1);
	int16_t sx2 = (int16_t)((DISPLAY_WIDTH / 2) + vx2) - 1;

	if (sx1 >= sx2 || sx2 <= 0 || sx1 >= DISPLAY_WIDTH)
		return;

	int16_t w1 = (int16_t)((CELL_SIZE / 2 * NEAR_PLANE * CAMERA_SCALE) / viewZ1);
	int16_t w2 = (int16_t)((CELL_SIZE / 2 * NEAR_PLANE * CAMERA_SCALE) / viewZ2);

#if WITH_TEXTURES
	DrawWallSegment(texture, sx1, w1, sx2, w2, u1, u2, edgeLeft, edgeRight, shadeEdge);
#else
	DrawWallSegment(sx1, w1, sx2, w2, edgeLeft, edgeRight, shadeEdge);
#endif
}

void swap(int16_t& a, int16_t& b)
{
	int16_t temp = a;
	a = b;
	b = temp;
}

void Renderer::DrawFloorLineInner(int16_t x0, int16_t y0, int16_t x1, int16_t y1)
{
	uint8_t color = COLOUR_BLACK;
	
	bool steep = ABS(y1 - y0) > ABS(x1 - x0);
	if (steep) 
	{
		swap(x0, y0);
		swap(x1, y1);
	}

	if (x0 > x1) 
	{
		swap(x0, x1);
		swap(y0, y1);
	}

	int16_t dx, dy;
	dx = x1 - x0;
	dy = ABS(y1 - y0);

	int16_t err = dx / 2;
	int8_t ystep;

	if (y0 < y1)
	{
		ystep = 1;
	}
	else
	{
		ystep = -1;
	}

	for (; x0 <= x1; x0++)
	{
		if(steep)
		{
			if(y0 >= 0 && y0 < DISPLAY_WIDTH && x0 >= 0 && x0 < DISPLAY_HEIGHT && x0 > GetHorizon(y0) + wBuffer[y0] && x0 > GetHorizon(y0) + 8)
			{
				Platform::PutPixel((uint8_t)y0, (uint8_t)x0 + horizonBuffer[y0] - HORIZON, color);
			}
		}
		else
		{
			if(x0 >= 0 && x0 < DISPLAY_WIDTH && y0 >= 0 && y0 < DISPLAY_HEIGHT && y0 > GetHorizon(x0) + wBuffer[x0] && y0 > GetHorizon(x0) + 8)
			{
				Platform::PutPixel((uint8_t)x0, (uint8_t)y0 + horizonBuffer[x0] - HORIZON, color);
			}
		}

		err -= dy;
		if (err < 0)
		{
			y0 += ystep;
			err += dx;
		}
	}
}

void Renderer::DrawFloorLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2)
{
	int16_t viewX1, viewZ1, viewX2, viewZ2;

	TransformToViewSpace(x1, y1, viewX1, viewZ1);
	TransformToViewSpace(x2, y2, viewX2, viewZ2);
	
	//if(viewX1 > viewX2)
	//{
	//	swap(viewX1, viewX2);
	//	swap(viewZ1, viewZ2);
	//}

	// Frustum cull
//	if (viewX2 < 0 && -2 * viewZ2 > viewX2)
//		return;
//	if (viewX1 > 0 && 2 * viewZ1 < viewX1)
//		return;

	// clip to the front pane
	if ((viewZ1 < CLIP_PLANE) && (viewZ2 < CLIP_PLANE))
		return;

	if (viewZ1 < CLIP_PLANE)
	{
		viewX1 += (CLIP_PLANE - viewZ1) * (viewX2 - viewX1) / (viewZ2 - viewZ1);
		viewZ1 = CLIP_PLANE;
	}
	else if (viewZ2 < CLIP_PLANE)
	{
		viewX2 += (CLIP_PLANE - viewZ2) * (viewX1 - viewX2) / (viewZ1 - viewZ2);
		viewZ2 = CLIP_PLANE;
	}

	// apply perspective projection
	int16_t vx1 = (int16_t)((int32_t)viewX1 * NEAR_PLANE * CAMERA_SCALE / viewZ1);
	int16_t vx2 = (int16_t)((int32_t)viewX2 * NEAR_PLANE * CAMERA_SCALE / viewZ2);

	// transform the end points into screen space
	int16_t sx1 = (int16_t)((DISPLAY_WIDTH / 2) + vx1);
	int16_t sx2 = (int16_t)((DISPLAY_WIDTH / 2) + vx2) - 1;

	//if (sx2 <= 0 || sx1 >= DISPLAY_WIDTH)
	//	return;

	int16_t w1 = (int16_t)((CELL_SIZE / 2 * NEAR_PLANE * CAMERA_SCALE) / viewZ1);
	int16_t w2 = (int16_t)((CELL_SIZE / 2 * NEAR_PLANE * CAMERA_SCALE) / viewZ2);

	DrawFloorLineInner(sx1, HORIZON + w1, sx2, HORIZON + w2);
}

void Renderer::DrawFloorLines()
{
	constexpr int size = 10;
	int16_t baseX = (Game::player.x - CELL_SIZE * size / 2) & 0xff00;
	int16_t baseY = (Game::player.y - CELL_SIZE * size / 2) & 0xff00;
	
	for(int n = 0; n < 10; n++)
	{
		DrawFloorLine(baseX, baseY + n * CELL_SIZE, baseX + CELL_SIZE * 10 - n * CELL_SIZE, baseY + CELL_SIZE * 10);
		DrawFloorLine(baseX, baseY + n * CELL_SIZE, baseX + n * CELL_SIZE, baseY);
	}
	
	for(int n = 1; n < 10; n++)
	{
		DrawFloorLine(baseX + n * CELL_SIZE, baseY, baseX + CELL_SIZE * 10, baseY + CELL_SIZE * 10 - n * CELL_SIZE);
		DrawFloorLine(baseX + n * CELL_SIZE, baseY + 10 * CELL_SIZE, baseX + 10 * CELL_SIZE, baseY + n * CELL_SIZE);
	}
}

void Renderer::DrawCell(uint8_t x, uint8_t y)
{
	CellType cellType = Map::GetCellSafe(x, y);

	if (isFrustrumClipped(x, y))
	{
		return;
	}

	switch (cellType)
	{
	case CellType::Torch:
	{
		const uint16_t* torchSpriteData = Game::globalTickFrame & 4 ? torchSpriteData1 : torchSpriteData2;
		constexpr uint8_t torchScale = 75;

		if (Map::IsSolid(x - 1, y))
		{
			DrawObject(torchSpriteData, x * CELL_SIZE + CELL_SIZE / 7, y * CELL_SIZE + CELL_SIZE / 2, torchScale, AnchorType::Center);
		}
		else if (Map::IsSolid(x + 1, y))
		{
			DrawObject(torchSpriteData, x * CELL_SIZE + 6 * CELL_SIZE / 7, y * CELL_SIZE + CELL_SIZE / 2, torchScale, AnchorType::Center);
		}
		else if (Map::IsSolid(x, y - 1))
		{
			DrawObject(torchSpriteData, x * CELL_SIZE + CELL_SIZE / 2, y * CELL_SIZE + CELL_SIZE / 7, torchScale, AnchorType::Center);
		}
		else if (Map::IsSolid(x, y + 1))
		{
			DrawObject(torchSpriteData, x * CELL_SIZE + CELL_SIZE / 2, y * CELL_SIZE + 6 * CELL_SIZE / 7, torchScale, AnchorType::Center);
		}
	}
	return;
	case CellType::Entrance:
		DrawObject(entranceSpriteData, x * CELL_SIZE + CELL_SIZE / 2, y * CELL_SIZE + CELL_SIZE / 2, 96, AnchorType::Ceiling);
		return;
	case CellType::Exit:
		DrawObject(exitSpriteData, x * CELL_SIZE + CELL_SIZE / 2, y * CELL_SIZE + CELL_SIZE / 2, 96);
		return;
	case CellType::Urn:
		DrawObject(urnSpriteData, x * CELL_SIZE + CELL_SIZE / 2, y * CELL_SIZE + CELL_SIZE / 2, 80);
		return;
	case CellType::Potion:
		DrawObject(potionSpriteData, x * CELL_SIZE + CELL_SIZE / 2, y * CELL_SIZE + CELL_SIZE / 2, 64);
		return;
	case CellType::Scroll:
		DrawObject(scrollSpriteData, x * CELL_SIZE + CELL_SIZE / 2, y * CELL_SIZE + CELL_SIZE / 2, 64);
		return;
	case CellType::Coins:
		DrawObject(coinsSpriteData, x * CELL_SIZE + CELL_SIZE / 2, y * CELL_SIZE + CELL_SIZE / 2, 64);
		return;
	case CellType::Crown:
		DrawObject(crownSpriteData, x * CELL_SIZE + CELL_SIZE / 2, y * CELL_SIZE + CELL_SIZE / 2, 64);
		return;
	case CellType::Sign:
		DrawObject(signSpriteData, x * CELL_SIZE + CELL_SIZE / 2, y * CELL_SIZE + CELL_SIZE / 2, 80);
		return;
	case CellType::Chest:
		DrawObject(chestSpriteData, x * CELL_SIZE + CELL_SIZE / 2, y * CELL_SIZE + CELL_SIZE / 2, 75);
		return;
	case CellType::ChestOpened:
		DrawObject(chestOpenSpriteData, x * CELL_SIZE + CELL_SIZE / 2, y * CELL_SIZE + CELL_SIZE / 2, 75);
		return;
	default:
		break;
	}

	if(numBufferSlicesFilled >= DISPLAY_WIDTH)
	{
		return;
	}

	if (!Map::IsSolid(x, y))
	{
		return;
	}
	
	int16_t x1 = x * CELL_SIZE;
	int16_t y1 = y * CELL_SIZE;
	int16_t x2 = x1 + CELL_SIZE;
	int16_t y2 = y1 + CELL_SIZE;

	bool blockedLeft = Map::IsSolid(x - 1, y);
	bool blockedRight = Map::IsSolid(x + 1, y);
	bool blockedUp = Map::IsSolid(x, y - 1);
	bool blockedDown = Map::IsSolid(x, y + 1);

#if WITH_IMAGE_TEXTURES
	const uint16_t* texture = wallTextureData + (16 * (cellType - 1));
#elif WITH_VECTOR_TEXTURES
	const uint8_t* texture = vectorTexture0; // (const uint8_t*) pgm_read_ptr(&textures[(uint8_t)cellType - (uint8_t)CellType::FirstSolidCell]);
#endif

	if (!blockedLeft && camera.x < x1)
	{
#if WITH_TEXTURES
		DrawWall(texture, x1, y1, x1, y2, !blockedUp && camera.y > y1, !blockedDown && camera.y < y2, true);
#else
		DrawWall(x1, y1, x1, y2, !blockedUp && camera.y > y1, !blockedDown && camera.y < y2, true);
#endif
	}

	if (!blockedDown && camera.y > y2)
	{
#if WITH_TEXTURES
		DrawWall(texture, x1, y2, x2, y2, !blockedLeft && camera.x > x1, !blockedRight && camera.x < x2, false);
#else
		DrawWall(x1, y2, x2, y2, !blockedLeft && camera.x > x1, !blockedRight && camera.x < x2, false);
#endif
	}

	if (!blockedRight && camera.x > x2)
	{
#if WITH_TEXTURES
		DrawWall(texture, x2, y2, x2, y1, !blockedDown && camera.y < y2, !blockedUp && camera.y > y1, true);
#else
		DrawWall(x2, y2, x2, y1, !blockedDown && camera.y < y2, !blockedUp && camera.y > y1, true);
#endif
	}

	if (!blockedUp && camera.y < y1)
	{
#if WITH_TEXTURES
		DrawWall(texture, x2, y1, x1, y1, !blockedRight && camera.x < x2, !blockedLeft && camera.x > x1, false);
#else
		DrawWall(x2, y1, x1, y1, !blockedRight && camera.x < x2, !blockedLeft && camera.x > x1, false);
#endif
	}
}

void Renderer::DrawCells()
{
	constexpr int8_t MAP_BUFFER_WIDTH = 16;
	constexpr int8_t MAP_BUFFER_HEIGHT = 16;
	
	int16_t cosAngle = FixedCos(camera.angle);
	int16_t sinAngle = FixedSin(camera.angle);

	int8_t bufferX = (int8_t)((camera.x + cosAngle * 7) >> 8) - MAP_BUFFER_WIDTH / 2;
	int8_t bufferY = (int8_t)((camera.y + sinAngle * 7) >> 8) - MAP_BUFFER_WIDTH / 2;; 
	
	if(bufferX < 0)
		bufferX = 0;
	if(bufferY < 0)
		bufferY = 0;
	if(bufferX > Map::width - MAP_BUFFER_WIDTH)
		bufferX = Map::width - MAP_BUFFER_WIDTH;
	if(bufferY > Map::height - MAP_BUFFER_HEIGHT)
		bufferY = Map::height - MAP_BUFFER_HEIGHT;
	
	// This should make cells draw front to back
	
	int8_t xd, yd;
	int8_t x1, y1, x2, y2;

	if(camera.rotCos > 0)
	{
		x1 = bufferX;
		x2 = x1 + MAP_BUFFER_WIDTH;
		xd = 1;
	}
	else
	{
		x2 = bufferX - 1;
		x1 = x2 + MAP_BUFFER_WIDTH;
		xd = -1;
	}
	if(camera.rotSin < 0)
	{
		y1 = bufferY;
		y2 = y1 + MAP_BUFFER_HEIGHT;
		yd = 1;
	}
	else
	{
		y2 = bufferY - 1;
		y1 = y2 + MAP_BUFFER_HEIGHT;
		yd = -1;
	}

	if(ABS(camera.rotCos) < ABS(camera.rotSin))
	{
		for(int8_t y = y1; y != y2; y += yd)
		{
			for(int8_t x = x1; x != x2; x+= xd)
			{
				DrawCell(x, y);
			}
		}
	}
	else
	{
		for(int8_t x = x1; x != x2; x+= xd)
		{
			for(int8_t y = y1; y != y2; y += yd)
			{
				DrawCell(x, y);
			}
		}
	}	
}

void DrawScaledOutline(const uint16_t* data, int8_t x, int8_t y, uint8_t halfSize, uint8_t inverseCameraDistance, uint8_t shiftAmount, bool invert)
{
    const uint8_t size = halfSize << 1;
    const uint8_t *lut = scaleLUT +
                         (((halfSize - 1) >> shiftAmount) *
                          ((halfSize - 1) >> shiftAmount));

    /* 裁剪到屏幕 */
    uint8_t i0 = (x < 0) ? (-x) : 0;
    uint8_t i1 = (x + size > DISPLAY_WIDTH) ? (DISPLAY_WIDTH - x) : size;
    uint8_t j0 = (y < 0) ? (-y) : 0;
    uint8_t j1 = (y + size > DISPLAY_HEIGHT) ? (DISPLAY_HEIGHT - y) : size;

    int8_t outX = x < 0 ? 0 : x;

    /* 预计算列缓存（与原来一致）*/
    uint16_t leftTC = 0, midT = 0, midC = 0, rightT = 0, rightC = 0;
    bool wasVisible = false;

    for (uint8_t i = i0; i < i1; ++i, ++outX)
    {
        if (Renderer::wBuffer[outX] >= inverseCameraDistance)
            continue;

        /* 本列要画，先准备列数据 */
        uint16_t outlineColumn = 0;
        if (i >= i1 - 2)
        {
            if (!wasVisible) break;
            leftTC = midC & midT;
            midC   = rightC;
            midT   = rightT;
            rightT = rightC = 0;
            outlineColumn = leftTC;
        }
        else
        {
            uint8_t u = lut[i >> shiftAmount];
            if (wasVisible)
            {
                leftTC = midC & midT;
                midC   = rightC;
                midT   = rightT;
                rightT = data[u * 2];
                rightC = data[u * 2 + 1] ^ (invert ? 0xFFFF : 0);
                outlineColumn = leftTC | (rightC & rightT);
            }
            else
            {
                leftTC = 0;
                rightT = data[u * 2];
                rightC = data[u * 2 + 1] ^ (invert ? 0xFFFF : 0);
                midC   = rightC;
                midT   = rightT;
                outlineColumn = (rightC & rightT);
            }
        }

        /* 逐像素画 */
        int8_t outY = y < 0 ? 0 : y;
        for (uint8_t j = j0; j < j1; ++j)
        {
            uint8_t v = lut[j >> shiftAmount];
            uint16_t mask = scaleDrawReadMasks[v];

            bool upOW   = (midC & midT) != 0 && (midC & mask) != 0;
            bool midO   = (midT & mask) != 0;
            bool midW   = (midC & mask) != 0;
            bool edge   = ((outlineColumn & mask) != 0) || upOW;
            bool draw   = midO && !edge;

            if (draw)
            {
                Platform::PutPixel(outX, outY + j, midW ? COLOUR_WHITE : COLOUR_BLACK);
            }
            else if (edge)
            {
                Platform::PutPixel(outX, outY + j, COLOUR_WHITE);
            }
        }
    }
}

template<int scaleMultiplier>
inline void DrawScaledNoOutline(const uint16_t* data, int8_t x, int8_t y, uint8_t halfSize, uint8_t inverseCameraDistance)
{
    const uint8_t size = halfSize << 1;
    const uint8_t *lut = scaleLUT +
                         ((halfSize / scaleMultiplier) *
                          (halfSize / scaleMultiplier));

    uint8_t i0 = (x < 0) ? (-x) : 0;
    uint8_t i1 = (x + size > DISPLAY_WIDTH) ? (DISPLAY_WIDTH - x) : size;
    uint8_t j0 = (y < 0) ? (-y) : 0;
    uint8_t j1 = (y + size > DISPLAY_HEIGHT) ? (DISPLAY_HEIGHT - y) : size;

    int8_t outX = x < 0 ? 0 : x;

    for (uint8_t i = i0; i < i1; ++i, ++outX)
    {
        if (Renderer::wBuffer[outX] >= inverseCameraDistance)
            continue;

        const uint8_t u = lut[i / scaleMultiplier];
        uint16_t transpCol = data[u * 2];
        uint16_t colorCol  = data[u * 2 + 1];

        int8_t outY = y < 0 ? 0 : y;
        for (uint8_t j = j0; j < j1; j += scaleMultiplier)
        {
            uint8_t v   = lut[j / scaleMultiplier];
            uint16_t m  = scaleDrawReadMasks[v];

            for (uint8_t k = 0; k < scaleMultiplier; ++k)
            {
                if ((transpCol & m) != 0)
                {
                    bool white = (colorCol & m) != 0;
                    Platform::PutPixel(outX, outY + j + k,
                                       white ? COLOUR_WHITE : COLOUR_BLACK);
                }
            }
        }
    }
}

void Renderer::DrawScaled(const uint16_t* data, int8_t x, int8_t y, uint8_t halfSize, uint8_t inverseCameraDistance, bool invert)
{
	// 处理缩放绘制
	if (halfSize > MAX_SPRITE_SIZE * 2)
	{
		return;
	}
	else if (halfSize > MAX_SPRITE_SIZE)
	{
		DrawScaledInner(data, x, y, halfSize, inverseCameraDistance, 2, invert);
	}
	else if (halfSize * 2 > MAX_SPRITE_SIZE)
	{
		DrawScaledInner(data, x, y, halfSize, inverseCameraDistance, 1, invert);
	}
	else if(halfSize > 2)
	{
		DrawScaledInner(data, x, y, halfSize, inverseCameraDistance, 0, invert);
	}
	else if (halfSize == 2)
	{
		if (Renderer::wBuffer[x] < inverseCameraDistance)
        {
            uint8_t colour = Platform::GetPixel((const uint8_t*)data, 0) ? COLOUR_WHITE : COLOUR_BLACK;
            Platform::PutPixel(x, y, colour);
            colour = Platform::GetPixel((const uint8_t*)data, 1) ? COLOUR_WHITE : COLOUR_BLACK;
            Platform::PutPixel(x, y + 1, colour);
        }
        if (Renderer::wBuffer[x + 1] < inverseCameraDistance)
        {
            uint8_t colour = Platform::GetPixel((const uint8_t*)data, 2) ? COLOUR_WHITE : COLOUR_BLACK;
            Platform::PutPixel(x + 1, y, colour);
            colour = Platform::GetPixel((const uint8_t*)data, 3) ? COLOUR_WHITE : COLOUR_BLACK;
            Platform::PutPixel(x + 1, y + 1, colour);
        }
	}
	else
	{
		if (Renderer::wBuffer[x] < inverseCameraDistance)
        {
            uint8_t colour = Platform::GetPixel((const uint8_t*)data, 0) ? COLOUR_WHITE : COLOUR_BLACK;
            Platform::PutPixel(x, y, colour);
        }
	}
}

void Renderer::DrawScaled16(const uint16_t* data, int8_t x, int8_t y, uint8_t halfSize, uint8_t inverseCameraDistance, bool invert) {
    const uint8_t spriteSize = halfSize * 2;

    for (int row = 0; row < spriteSize; row++) {
        uint16_t rowData = data[row]; // 获取当前行数据（MSB优先）

        for (int col = 0; col < spriteSize; col++) {
            // MSB优先解析：从最高位(bit15)到最低位(bit0)
            bool isWhite = (rowData & (0x8000 >> col)) != 0; // 关键修改：使用右移
            
            // 或者等效写法：
            // bool isWhite = (rowData & (1 << (15 - col))) != 0;
            
            uint8_t color = isWhite ? COLOUR_WHITE : COLOUR_BLACK;
            if (invert) color ^= 0x1; // 反色处理

            // 带深度检测的绘制
            int8_t px = x + col;
            int8_t py = y + row;
            if (px >= 0 && px < DISPLAY_WIDTH && 
                py >= 0 && py < DISPLAY_HEIGHT &&
                Renderer::wBuffer[px] < inverseCameraDistance) {
                Platform::PutPixel(px, py, color);
            }
        }
    }
}

void Renderer::DrawScaled8(const uint8_t* data, int8_t x, int8_t y, uint8_t halfSize, uint8_t inverseCameraDistance, bool invert) {
    const uint8_t spriteSize = halfSize * 2;
    const uint8_t bytesPerRow = (spriteSize + 7) / 8; // 每行字节数

    for (uint8_t row = 0; row < spriteSize; row++) {
        for (uint8_t colByte = 0; colByte < bytesPerRow; colByte++) {
            uint8_t byteData = data[row * bytesPerRow + colByte]; // 获取当前字节
            
            // 处理当前字节的8个像素（MSB优先）
            for (uint8_t bit = 0; bit < 8; bit++) {
                uint8_t pixelX = colByte * 8 + bit;
                if (pixelX >= spriteSize) break; // 超出精灵宽度则跳过
                
                // 提取像素（MSB对应最左侧像素）
                bool isWhite = (byteData & (0x80 >> bit)) != 0;
                uint8_t color = (isWhite ^ invert) ? COLOUR_WHITE : COLOUR_BLACK;
                
                // 计算目标坐标并绘制
                int8_t targetX = x + pixelX;
                int8_t targetY = y + row;
                if (targetX >= 0 && targetX < DISPLAY_WIDTH && 
                    targetY >= 0 && targetY < DISPLAY_HEIGHT &&
                    Renderer::wBuffer[targetX] < inverseCameraDistance) {
                    Platform::PutPixel(targetX, targetY, color);
                }
            }
        }
    }
}

QueuedDrawable* Renderer::CreateQueuedDrawable(uint8_t inverseCameraDistance)
{
	uint8_t insertionPoint = MAX_QUEUED_DRAWABLES;
	
	for(uint8_t n = 0; n < numQueuedDrawables; n++)
	{
		if(inverseCameraDistance < queuedDrawables[n].inverseCameraDistance)
		{
			if(numQueuedDrawables < MAX_QUEUED_DRAWABLES)
			{
				insertionPoint = n;
				numQueuedDrawables++;
				
				for (uint8_t i = numQueuedDrawables - 1; i > n; i--)
				{
					queuedDrawables[i] = queuedDrawables[i - 1];
				}
			}
			else
			{
				if(n == 0)
				{
					// List is full and this is smaller than the first element so just cull
					return nullptr;
				}
				
				// Drop the smallest element to make a space
				for (uint8_t i = 0; i < n - 1; i++)
				{
					queuedDrawables[i] = queuedDrawables[i + 1];
				}
				
				insertionPoint = n - 1;
			}
			
			break;
		}
	}
	
	if(insertionPoint == MAX_QUEUED_DRAWABLES)
	{
		if(numQueuedDrawables < MAX_QUEUED_DRAWABLES)
		{
			insertionPoint = numQueuedDrawables;
			numQueuedDrawables++;
		}
		else if (inverseCameraDistance > queuedDrawables[numQueuedDrawables - 1].inverseCameraDistance)
		{
			// Drop the smallest element to make a space
			for (uint8_t i = 0; i < numQueuedDrawables - 1; i++)
			{
				queuedDrawables[i] = queuedDrawables[i + 1];
			}
			insertionPoint = numQueuedDrawables - 1;
		}
		else
		{
			return nullptr;
		}
	}
	
	return &queuedDrawables[insertionPoint];
}

void Renderer::QueueSprite(const uint16_t* data, int8_t x, int8_t y, uint8_t halfSize, uint8_t inverseCameraDistance, bool invert)
{
	if(x < -halfSize * 2)
		return;
	//if(x >= DISPLAY_WIDTH)
	//	return;
	//if(halfSize <= 2)
	//	return;

	QueuedDrawable* drawable = CreateQueuedDrawable(inverseCameraDistance);
	
	if(drawable != nullptr)
	{
		drawable->type = DrawableType::Sprite;
		drawable->spriteData = data;
		drawable->x = x;
		drawable->y = y;
		drawable->halfSize = halfSize;
		drawable->inverseCameraDistance = inverseCameraDistance;
		drawable->invert = invert;
	}
}

void Renderer::RenderQueuedDrawables()
{
	for(uint8_t n = 0; n < numQueuedDrawables; n++)
	{
		QueuedDrawable& drawable = queuedDrawables[n];
		
		if(drawable.type == DrawableType::Sprite)
		{
			DrawScaled(drawable.spriteData, drawable.x, drawable.y, drawable.halfSize, drawable.inverseCameraDistance, drawable.invert);
		}
		else
		{
			drawable.particleSystem->Draw(drawable.x, drawable.inverseCameraDistance);
		}
	}
}

int8_t Renderer::GetHorizon(int16_t x)
{
	if (x < 0)
		x = 0;
	if (x >= DISPLAY_WIDTH)
		x = DISPLAY_WIDTH - 1;
	return horizonBuffer[x];
}

bool Renderer::TransformAndCull(int16_t worldX, int16_t worldY, int16_t& outScreenX, int16_t& outScreenW)
{
	int16_t relX, relZ;
	TransformToViewSpace(worldX, worldY, relX, relZ);

	// Frustum cull
	if (relZ < CLIP_PLANE)
		return false;

	if (relX < 0 && -2 * relZ > relX)
		return false;
	if (relX > 0 && 2 * relZ < relX)
		return false;

	TransformToScreenSpace(relX, relZ, outScreenX, outScreenW);
	
	return true;
}

void Renderer::DrawObject(const uint16_t* spriteData, int16_t x, int16_t y, uint8_t scale, AnchorType anchor, bool invert)
{
	int16_t screenX, screenW;

	if(TransformAndCull(x, y, screenX, screenW))
	{
		// Bit of a hack: nudge sorting closer to the camera
		uint8_t inverseCameraDistance = (uint8_t)(screenW + 1);
		int16_t spriteSize = (screenW * scale) / 128;
		int8_t outY = GetHorizon(screenX);

		switch (anchor)
		{
		case AnchorType::Floor:
			outY += screenW - 2 * spriteSize;
			break;
		case AnchorType::Center:
			outY -= spriteSize;
			break;
		case AnchorType::BelowCenter:
			break;
		case AnchorType::Ceiling:
			outY -= screenW;
			break;
		}
		
		QueueSprite(spriteData, screenX - spriteSize, outY, (uint8_t)spriteSize, inverseCameraDistance, invert);
	}
}

void Renderer::DrawWeapon()
{
	int x = DISPLAY_WIDTH / 2 + 22 + camera.tilt / 4;
	int y = DISPLAY_HEIGHT - 21 - camera.bob;
	uint8_t reloadTime = Game::player.reloadTime;
	
	if(reloadTime > 0)
	{
		Platform::DrawSprite(x - reloadTime / 3 - 1, y - reloadTime / 3 - 6, handSpriteData2, 0);
		//DrawSprite(x - reloadTime / 3 - 1, y - reloadTime / 3 - 1, handSpriteData2, handSpriteData2_mask, 0, 0);	
	}
	else
	{
		Platform::DrawSprite(x + 2, y - 3, handSpriteData1, 0);
		//DrawSprite(x + 2, y + 2, handSpriteData1, handSpriteData1_mask, 0, 0);	
	}
	
}

void Renderer::DrawBackground()
{
	// 纯黑色天空（覆盖上半屏）
    u8g2_SetDrawColor(&screen, COLOUR_BLACK); // 设置为黑色
    u8g2_DrawBox(&screen, 0, 0, DISPLAY_WIDTH, HORIZON);
    
    // 保留原有的地面绘制（如果需要）
    u8g2_SetDrawColor(&screen, COLOUR_WHITE);
    u8g2_DrawBox(&screen, 0, HORIZON, DISPLAY_WIDTH, DISPLAY_HEIGHT - HORIZON);

    /* 上半屏 32 行：棋盘格 0x55 / 0x00 / 0xAA / 0x00 */
    //for (uint8_t y = 0; y < 32; y += 4)
    //{
    //    u8g2_DrawHLine(&screen, 0, y,     128);   /* 0x55 → 01010101 用 XOR 画 */
    //    u8g2_DrawHLine(&screen, 0, y + 1, 128);
    //}

    /* 下半屏 32 行：0x55 / 0xFF / 0xAA / 0xFF */
    //u8g2_SetDrawColor(&screen, COLOUR_WHITE);
    //u8g2_DrawBox(&screen, 0, 32, 128, 32);        /* 整片白 */
}

void Renderer::DrawBar(const uint8_t *iconData, uint8_t amount, uint8_t max, uint8_t type)
{
    constexpr uint8_t ICON_W = 8, ICON_H = 8;
    constexpr uint8_t BAR_W  = 40;

    uint8_t fill = (uint32_t)amount * BAR_W / max;

	if (type == 1)
	{
		/* 血条固定画在 (0,56) */
    	u8g2_SetDrawColor(&screen, COLOUR_WHITE);
    	u8g2_DrawXBM(&screen, 0, 56, ICON_W, ICON_H, iconData);

		u8g2_SetDrawColor(&screen, COLOUR_BLACK);
		u8g2_DrawBox(&screen, ICON_W, 56, BAR_W, 8);

		u8g2_SetDrawColor(&screen, COLOUR_WHITE);
    	u8g2_DrawBox(&screen, ICON_W, 56, fill, 8);
		u8g2_SetDrawColor(&screen, COLOUR_BLACK);
		u8g2_DrawFrame(&screen, ICON_W, 56, BAR_W, 8);
	}
    else if (type == 2)
	{
		/* 蓝条固定画在 (0,48) */
		u8g2_SetDrawColor(&screen, COLOUR_WHITE);
    	u8g2_DrawXBM(&screen, 0, 48, ICON_W, ICON_H, iconData);

		u8g2_SetDrawColor(&screen, COLOUR_WHITE);
		u8g2_DrawBox(&screen, ICON_W, 48, BAR_W, 8);
	
		u8g2_SetDrawColor(&screen, COLOUR_BLACK);
    	u8g2_DrawBox(&screen, ICON_W, 48, fill, 8);
		u8g2_SetDrawColor(&screen, COLOUR_BLACK);
		u8g2_DrawFrame(&screen, ICON_W, 48, BAR_W, 8);
	}
}

void Renderer::DrawDamageIndicator()
{
    /* 上、下两行像素清 0 */
    u8g2_SetDrawColor(&screen, COLOUR_BLACK);
    u8g2_DrawHLine(&screen, 1, 0,             126);
    u8g2_DrawHLine(&screen, 1, DISPLAY_HEIGHT - 1, 126);

    /* 左、右两条竖线清 0 */
    u8g2_DrawVLine(&screen, 0, 0, DISPLAY_HEIGHT);
    u8g2_DrawVLine(&screen, DISPLAY_WIDTH - 1, 0, DISPLAY_HEIGHT);
}

void Renderer::DrawHUD()
{
    /* 直接画在缓冲上，不擦除背景 */
    /* 血条 */
    DrawBar(heartSpriteData, Game::player.hp, Game::player.maxHP, 1);

    /* 蓝条 */
    DrawBar(manaSpriteData, Game::player.mana, Game::player.maxMana, 2);

    /* 受伤闪边框 */
    if (Game::player.damageTime > 0)
        DrawDamageIndicator();

    /* 文字提示 */
    if (Game::displayMessage)
        Font::PrintString(Game::displayMessage, 0, 0, 0);
}

void Renderer::Render()
{
	//u8g2_ClearBuffer(&screen);

	globalRenderFrame++;

	DrawBackground();

	numBufferSlicesFilled = 0;
	numQueuedDrawables = 0;
	
	for (uint8_t n = 0; n < DISPLAY_WIDTH; n++)
	{
		wBuffer[n] = 0;
		horizonBuffer[n] = HORIZON + (((DISPLAY_WIDTH / 2 - n) * camera.tilt) >> 8) + camera.bob;
	}

	camera.cellX = camera.x / CELL_SIZE;
	camera.cellY = camera.y / CELL_SIZE;

	camera.rotCos = FixedCos(-camera.angle);
	camera.rotSin = FixedSin(-camera.angle);
	camera.clipCos = FixedCos(-camera.angle + CLIP_ANGLE);
	camera.clipSin = FixedSin(-camera.angle + CLIP_ANGLE);

	DrawCells();
	//DrawFloorLines();

	EnemyManager::Draw();
	ProjectileManager::Draw();
	ParticleSystemManager::Draw();
	
	RenderQueuedDrawables();

	DrawWeapon();

	DrawHUD();

	Map::DrawMinimap();
}
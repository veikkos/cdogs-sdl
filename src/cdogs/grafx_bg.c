/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (c) 2013-2014, 2016-2019 Cong Xu
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.
    Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
    ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
    LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
    INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
    CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
    ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
    POSSIBILITY OF SUCH DAMAGE.
*/
#include "grafx_bg.h"

#include "actors.h"
#include "ai.h"
#include "draw/draw.h"
#include "draw/drawtools.h"
#include "game_events.h"
#include "handle_game_events.h"
#include "log.h"
#include "map_build.h"
#include "objs.h"
#include "pickup.h"
#include "quick_play.h"
#include "texture.h"
#include "triggers.h"


void GrafxMakeRandomBackground(
	GraphicsDevice *device,
	CampaignOptions *co, struct MissionOptions *mo, Map *map)
{
	CampaignSettingInit(&co->Setting);
	SetupQuickPlayCampaign(&co->Setting);
	const HSV tint = {
		rand() * 360.0 / RAND_MAX, rand() * 1.0 / RAND_MAX, 0.5
	};
	DrawBuffer buffer;
	DrawBufferInit(&buffer, svec2i(X_TILES, Y_TILES), device);
	co->MissionIndex = 0;
	GrafxMakeBackground(
		device, &buffer, co, mo, map, tint, false, svec2_zero(), NULL);
	DrawBufferTerminate(&buffer);
	MissionOptionsTerminate(mo);
	CampaignSettingTerminate(&co->Setting);
}

static void DrawBackgroundWithRenderer(
	GraphicsDevice *g, WindowContext *wc, SDL_Texture *target,
	SDL_Texture *src, DrawBuffer *buffer,
	const HSV tint, const struct vec2 pos, GrafxDrawExtra *extra);
void GrafxDrawBackground(
	GraphicsDevice *g, DrawBuffer *buffer,
	const HSV tint, const struct vec2 pos, GrafxDrawExtra *extra)
{
	DrawBackgroundWithRenderer(
		g, &g->gameWindow, g->bkgTgt, g->bkg, buffer, tint, pos, extra);
	if (g->cachedConfig.SecondWindow)
	{
		DrawBackgroundWithRenderer(
			g, &g->secondWindow, g->bkgTgt2, g->bkg2, buffer, tint, pos,
			extra);
	}
}
static void DrawBackground(
	GraphicsDevice *g, SDL_Texture *t, DrawBuffer *buffer,
	Map *map, const struct vec2 pos, GrafxDrawExtra *extra);
static void DrawBackgroundWithRenderer(
	GraphicsDevice *g, WindowContext *wc, SDL_Texture *target,
	SDL_Texture *src, DrawBuffer *buffer,
	const HSV tint, const struct vec2 pos, GrafxDrawExtra *extra)
{
	SDL_RendererInfo ri;
	if (SDL_GetRendererInfo(wc->renderer, &ri) != 0)
	{
		LOG(LM_GFX, LL_ERROR, "cannot set render target: %s", SDL_GetError());
	}
	else
	{
		if (!(ri.flags & SDL_RENDERER_TARGETTEXTURE))
		{
			LOG(LM_GFX, LL_ERROR,
				"renderer does not support render to texture");
		}
	}
	if (SDL_SetRenderTarget(wc->renderer, target) != 0)
	{
		LOG(LM_GFX, LL_ERROR, "cannot set render target: %s", SDL_GetError());
	}
	wc->bkgMask = ColorTint(colorWhite, tint);
	DrawBackground(g, src, buffer, &gMap, pos, extra);
	if (SDL_SetRenderTarget(wc->renderer, NULL) != 0)
	{
		LOG(LM_GFX, LL_ERROR, "cannot set render target: %s", SDL_GetError());
	}
}
static void DrawBackground(
	GraphicsDevice *g, SDL_Texture *t, DrawBuffer *buffer,
	Map *map, const struct vec2 pos, GrafxDrawExtra *extra)
{
	BlitClearBuf(g);
	DrawBufferSetFromMap(buffer, map, pos, X_TILES);
	DrawBufferDraw(buffer, svec2i_zero(), extra);
	BlitUpdateFromBuf(g, t);
	BlitClearBuf(g);
}

void GrafxRedrawBackground(GraphicsDevice *g, const struct vec2 pos)
{
	memset(g->buf, 0, GraphicsGetMemSize(&g->cachedConfig));
	DrawBuffer buffer;
	DrawBufferInit(&buffer, svec2i(X_TILES, Y_TILES), g);
	const HSV tint = {
		rand() * 360.0 / RAND_MAX, rand() * 1.0 / RAND_MAX, 0.5
	};
	GrafxDrawBackground(g, &buffer, tint, pos, NULL);
	DrawBufferTerminate(&buffer);
}

void GrafxMakeBackground(
	GraphicsDevice *device, DrawBuffer *buffer,
	CampaignOptions *co, struct MissionOptions *mo, Map *map, HSV tint,
	const bool isEditor, struct vec2 pos, GrafxDrawExtra *extra)
{
	CampaignAndMissionSetup(co, mo);
	GameEventsInit(&gGameEvents);
	MapBuild(map, mo->missionData, co);
	InitializeBadGuys();
	CreateEnemies();
	MapMarkAllAsVisited(map);
	if (isEditor)
	{
		MapShowExitArea(map, map->ExitStart, map->ExitEnd);
	}
	else
	{
		pos = Vec2CenterOfTile(svec2i_scale_divide(map->Size, 2));
	}
	// Process the events that place dynamic objects
	HandleGameEvents(&gGameEvents, NULL, NULL, NULL);
	GrafxDrawBackground(device, buffer, tint, pos, extra);
	GameEventsTerminate(&gGameEvents);
}

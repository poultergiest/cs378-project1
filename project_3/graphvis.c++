#include <iostream>
#include <vector>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <cassert>
#include <SDL/SDL.h>
#include "drawing.h"

bool doExit = false;

using namespace std;

#define HOOKES_K -2

class COORD {
public:
	COORD(int i, int j) {
		x = i;
		y = j;
	}
	int x;
	int y;
};

// insert graph
class AdjGraph {
	int _size;
	vector<vector<bool> > _g;
	vector<int> label;
	vector<COORD> node_pos;
public:
	AdjGraph(int n) {
		_size = n;
		_g.resize(_size);
		label.resize(_size);
		for(int i = 0; i < _size; ++i) {
			_g[i].resize(_size);
			label[i] = INT_MAX;
		}
	}

	void setLabel(int node, int value) {
		label[node] = value;
	}

	int getLabel(int node) {
		return label[node];
	}

	bool getEdge(int node1, int node2) {
		return _g[node1][node2];
	}

	void setEdge(int node1, int node2, bool value) {
		_g[node1][node2] = value;
	}

	COORD getCoord(int node) {
		return node_pos[node];
	}

	void setCoord(int node, int i, int j) {
		node_pos[node].x = i;
		node_pos[node].y = j;
	}

	void addNode(int x, int y) {
		_size++;
		_g.resize(_size);
		label.push_back(INT_MAX);
		COORD pos(x,y);
		node_pos.push_back(pos);
		for(int i = 0; i < _size; ++i) _g[i].resize(_size);
	}

	vector<int> getNeighbors(int node) {
		vector<int> n;
		for(int i = 0; i < _size; ++i) {
			if(node == i) continue;
			if(_g[node][i]) {
				n.push_back(i);
			}
		}
		return n;
	}

	vector<int> getBiggerNeighbors(int node) {
		vector<int> n;
		for(int i = node; i < _size; ++i) {
			if(node == i) continue;
			if(_g[node][i]) {
				n.push_back(i);
			}
		}
		return n;
	}

	int getSize() { return _size; }

	void print() {
		for(int x = 0; x < _size; ++x) {
			for(int y = 0; y < _size; ++y) {
				cout << getEdge(x,y) << " ";
			}
			cout << endl;
		}
	}

	void printNodeCoords(int node) {
		cout << node << ": (" << node_pos[node].x << ", " << node_pos[node].y << ")" << endl;
	}

	void printCoords() {
		for(int i = 0; i < _size; ++i) {
			printNodeCoords(i);
		}
	}
};


// implement forces
int gdistance(COORD n1, COORD n2) {
	int x = (n1.x - n2.x) * (n1.x - n2.x);
	int y = (n1.y - n2.y) * (n1.y - n2.y);
	return (int) sqrt(x+y);
}

COORD hookes_force(COORD src, COORD dest) {
	COORD res(0,0);

	int d = gdistance(src, dest);

	res.x = -1 * HOOKES_K * (dest.x - src.x) / d;
	res.y = -1 * HOOKES_K * (dest.y - src.y) / d;

	return res;
}

void apply_hookes(AdjGraph& g) {
	for(int i = 0; i < g.getSize(); ++i) {
		COORD src = g.getCoord(i);
		vector<int> neighbors = g.getNeighbors(i);
		for(int j = 0; j < (int) neighbors.size(); ++j) {
			COORD dest = g.getCoord(neighbors[j]);
			COORD hookes = hookes_force(src, dest);
			g.setCoord(i, src.x + hookes.x, src.y + hookes.y);
		}
	}
}


//Perform graph layout, Daniel Tunkelang style
static bool init_app(const char * name, SDL_Surface * icon, uint32_t flags)
{
  atexit(SDL_Quit);
  if(SDL_Init(flags) < 0)
    return 0;

  SDL_WM_SetCaption(name, name);
  SDL_WM_SetIcon(icon, NULL);

  return 1;
}

static void init_data(struct rgbData data[][WIDTH])
{
  memset(data, 255, WIDTH*HEIGHT*sizeof(rgbData));
}

static void render(SDL_Surface * sf)
{
  SDL_Surface * screen = SDL_GetVideoSurface();
  if(SDL_BlitSurface(sf, NULL, screen, NULL) == 0)
    SDL_UpdateRect(screen, 0, 0, 0, 0);
}

static int filter(const SDL_Event * event)
{ 
	if (event->type == SDL_QUIT)
		doExit = true;
	return event->type == SDL_QUIT;
}

static bool renderGraph(struct rgbData data[][WIDTH], unsigned width, unsigned height, AdjGraph& g, int which, bool forces)
{
	for(SDL_Event event; SDL_PollEvent(&event);)
		if(event.type == SDL_QUIT) return 0;

	rgbData black = {0,0,0};
	rgbData white = {255,255,255};
	rgbData red = {255,0,0};
	rgbData green = {0,255,0};
	rgbData blue = {0,0,255};
	rgbData color = {200,70,110};

	colorBG(data, white); 

	// DRAW NODES
	for(int i = 0; i < g.getSize(); ++i) {
		COORD node = g.getCoord(i);
		drawcircle(data, node.x, node.y, 10, red);
		char buffer[4];
		snprintf(buffer, sizeof(buffer), "%d", i);
		drawstring(data, node.x-2, node.y-2, buffer, black);
	}

	// DRAW EDGES
	for(int i = 0; i < g.getSize(); ++i) {
		COORD src = g.getCoord(i);
		vector<int> neighbors = g.getNeighbors(i);
		for(int j = 0; j < (int) neighbors.size(); ++j) {
			COORD dest = g.getCoord(neighbors[j]);
			drawline(data, src.x, src.y, dest.x, dest.y, black);
		}
	}
	return 1;
}

int main(int argc, char **argv) {

	//Initialize visualization
	struct rgbData buffer[HEIGHT][WIDTH];

	bool ok =
	init_app("SDL example", NULL, SDL_INIT_VIDEO) &&
	SDL_SetVideoMode(WIDTH, HEIGHT, 24, SDL_HWSURFACE);

	assert(ok);

	SDL_Surface * data_sf = 
	SDL_CreateRGBSurfaceFrom((char*)buffer, WIDTH, HEIGHT, 24, WIDTH * 3,
	                     0x000000FF, 0x0000FF00, 0x00FF0000, 0);

	SDL_SetEventFilter(filter);

	init_data(buffer);

	int rs = 3;
	AdjGraph ring(0);
	ring.addNode(100, 100);
	ring.addNode(200, 100);
	ring.addNode(200, 200);
	
	for(int n = 0; n < rs-1; ++n) {
		ring.setEdge(n, n+1, 1);
	}
	ring.setEdge(rs-1, 0, 1);

	ring.print();

	while (!doExit) {
		//apply laws
		//ring.printCoords();
		apply_hookes(ring);
		//cout << endl;
		//ring.printCoords();
		//render
		renderGraph(buffer, WIDTH, HEIGHT, ring, 0, false);
		render(data_sf);
		SDL_Delay(1000/30);
	}
	return 0;
}

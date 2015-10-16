#ifndef SCENE_GRAPH_H
#define SCENE_GRAPH_H

#include "CoreModel.h"

class SceneGraph {
public:
	SceneGraph();
	int count();
	void add(Model* node);
	Model* at(int i);
	void reclaim();
private:
	void resize();
	Model** objects;
	int _count;
	int _capacity;
};

#endif

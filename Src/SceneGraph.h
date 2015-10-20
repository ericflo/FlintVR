#ifndef SCENE_GRAPH_H
#define SCENE_GRAPH_H

#include "CoreModel.h"

class SceneGraph {
public:
	SceneGraph();
	int count();
	void add(CoreModel* node);
  int indexOf(CoreModel* node);
  void remove(int i);
	CoreModel* at(int i);
	void reclaim();
private:
	void resize();
	CoreModel** objects;
	int _count;
	int _capacity;
};

#endif

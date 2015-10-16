#include "SceneGraph.h"

SceneGraph::SceneGraph(void) {
	resize();
}

int SceneGraph::count() {
	return _count;
}

void SceneGraph::add(Model* node) {
	if (_count == _capacity) {
		resize();
	}
	objects[_count++] = node;
}

void SceneGraph::resize() {
	int nextCapacity;
  if (_count == 0) {
    nextCapacity = 10;
  } else {
    nextCapacity = _capacity * 2;
  }
  Model** nextObjects = new Model*[_capacity];
  for (int i = 0; i < _count; ++i) {
    nextObjects[i] = objects[i];
    objects[i] = NULL;
  }
  if (_count != 0) {
  	reclaim();
  }
  objects = nextObjects;
  _capacity = nextCapacity;
}

Model* SceneGraph::at(int i) {
	return objects[i];
}

void SceneGraph::reclaim() {
	delete[] objects;
}

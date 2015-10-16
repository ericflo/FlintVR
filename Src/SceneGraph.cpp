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

int SceneGraph::indexOf(Model* node) {
  for (int i = 0; i < _count; ++i) {
    if (objects[i]->id == node->id) {
      return i;
    }
  }
  return -1;
}

void SceneGraph::remove(int i) {
  _count--;
  for (int j = i; j < _count; ++j) {
    objects[j] = objects[j+1];
  }
}

void SceneGraph::resize() {
	int nextCapacity;
  if (_count == 0) {
    nextCapacity = 1;
  } else {
    nextCapacity = _capacity * 2;
  }
  Model** nextObjects = new Model*[nextCapacity];
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

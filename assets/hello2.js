import React, {Component} from 'react';
import {Model, Vector3f, Vector4f, Scene, VERTEX_POSITION, VERTEX_COLOR} from 'vrnet';

const CUBE_VERTEX = `#version 300 es
in vec3 Position;
in vec4 VertexColor;
uniform mat4 Modelm;
uniform mat4 Viewm;
uniform mat4 Projectionm;
out vec4 fragmentColor;
void main()
{
 gl_Position = Projectionm * ( Viewm * ( Modelm * vec4( Position, 1.0 ) ) );
 fragmentColor = VertexColor;
}`

const CUBE_FRAGMENT = `#version 300 es
in lowp vec4 fragmentColor;
out lowp vec4 outColor;
void main()
{
 outColor = fragmentColor;
}`

class Cube extends Model {
  geometry = {
    vertices: [
      VERTEX_POSITION,       VERTEX_COLOR,
      Vector3f(-1,  1, -1),  Vector4f(1, 0, 1, 1), // top
      Vector3f( 1,  1, -1),  Vector4f(0, 1, 0, 1),
      Vector3f( 1,  1,  1),  Vector4f(0, 0, 1, 1),
      Vector3f(-1,  1,  1),  Vector4f(1, 0, 0, 1),
      Vector3f(-1, -1, -1),  Vector4f(0, 0, 1, 1), // bottom
      Vector3f(-1, -1,  1),  Vector4f(0, 1, 0, 1),
      Vector3f( 1, -1,  1),  Vector4f(1, 0, 1, 1),
      Vector3f( 1, -1, -1),  Vector4f(1, 0, 0, 1)
    ],
    indices: [
      0, 1, 2, 2, 3, 0, // top
      4, 5, 6, 6, 7, 4, // bottom
      2, 6, 7, 7, 1, 2, // right
      0, 4, 5, 5, 3, 0, // left
      3, 5, 6, 6, 2, 3, // front
      0, 1, 7, 7, 4, 0  // back
    ]
  }
  program = {
    vertex: CUBE_VERTEX,
    fragment: CUBE_FRAGMENT
  }
}

class App extends Component {
  state = {
    start: (new Date()).getTime()
  }

  onFrame() {
    var secondsElapsed = ((new Date()).getTime() - this.state.start) / 1000.0;
    this.refs.cube.rotation.x = secondsElapsed;
    this.refs.cube.rotation.y = secondsElapsed * 0.8;
  }

  render() {
    return (
      <Scene clearColor={Vector4f(1, 1, 1, 1)}>
        <Cube ref="cube" />
      </Scene>
    );
  }
}

function frame(env) {}

function vrmain(env) {
  const app = render(<App />, env.scene);
  return frame;
}
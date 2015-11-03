function vrmain(env) {

var Geometry        = env.core.Geometry;
var Program         = env.core.Program;
var Model           = env.core.Model;
var Texture         = env.core.Texture;
var Vector3f        = env.core.Vector3f;
var Vector4f        = env.core.Vector4f;
var Matrix4f        = env.core.Matrix4f;
var VERTEX_POSITION = env.core.VERTEX_POSITION;
////////////////////////////////////////////

// Constants
var SCREEN_MENU = 1;
var SCREEN_GAME = 2;

// Actions
var ACTION_FRAME = 1;
var ACTION_START_GAME = 2;
var ACTION_CANCEL_GAME = 3;
var ACTION_SPAWN_ENEMY = 4;

var state = {
  screen:  SCREEN_MENU,
  level:   1,
  start:   0,
  elapsed: 0,
  menu:    new Menu(),
  game:    new Game(),
  enemies: [],
  spawned: 0,
};

function handleFrame(now) {
  // Update time counters
  if (state.start === 0) {
    state.start = now;
  }
  state.elapsed = now - state.start;

  // Perform any time based actions
  if (state.screen === SCREEN_GAME &&         // If we're playing the game
      state.level === 1 &&                    // And we're on level 1
      60 * state.spawned <= state.elapsed &&  // Then every 60 seconds
      state.spawned <= 20) {                  // Unless we've already spawned 20
    handleAction(ACTION_SPAWN_ENEMY);  // Spawn an enemy
  }

  // Update positions
  for (var i = 0; i < state.enemies.length; ++i) {
    state.enemies[i].update(i, state.elapsed);
  }
}

function handleAction(act, data) {
  switch(act) {
  case ACTION_FRAME:
    handleFrame(data);
    break;
  case ACTION_START_GAME:
    state.screen = SCREEN_GAME;
    state.level = 1;
    state.spawned = 0;
    env.scene.remove(state.menu.model);
    env.scene.add(state.game.model);
    break;
  case ACTION_CANCEL_GAME:
    state.screen = SCREEN_MENU;
    state.level = 1;
    env.scene.remove(state.game.model);
    env.scene.add(state.menu.model);
    break;
  case ACTION_SPAWN_ENEMY:
    var enemy = Enemy();
    state.spawned += 1;
    state.enemies.push(enemy);
    state.game.add(enemy.model);
    break;
  }
}

var colorProgram = Program((
  '#version 300 es\n'+
  'in vec3 Position;\n'+
  'in vec4 VertexColor;\n'+
  'uniform mat4 Modelm;\n'+
  'uniform mat4 Viewm;\n'+
  'uniform mat4 Projectionm;\n'+
  'out vec4 fragmentColor;\n'+
  'void main()\n'+
  '{\n'+
  ' gl_Position = Projectionm * (Viewm * (Modelm * vec4(Position, 1.0)));\n'+
  ' fragmentColor = VertexColor;\n'+
  '}'
), (
  '#version 300 es\n'+
  'in lowp vec4 fragmentColor;\n'+
  'out lowp vec4 outColor;\n'+
  'void main()\n'+
  '{\n'+
  ' outColor = fragmentColor;\n'+
  '}'
));

function Box(size) {
  size = size || 1;
  var s = size / 2.0;
  var cubeVertices = [
    VERTEX_POSITION,     
    Vector3f(-s,  s, -s),
    Vector3f( s,  s, -s),
    Vector3f( s,  s,  s),
    Vector3f(-s,  s,  s),
    Vector3f(-s, -s, -s),
    Vector3f(-s, -s,  s),
    Vector3f( s, -s,  s),
    Vector3f( s, -s, -s)
  ];
  var cubeIndices = [
    0, 1, 2, 2, 3, 0, // top
    4, 5, 6, 6, 7, 4, // bottom
    2, 6, 7, 7, 1, 2, // right
    0, 4, 5, 5, 3, 0, // left
    3, 5, 6, 6, 2, 3, // front
    0, 1, 7, 7, 4, 0  // back
  ];
  return Geometry({
    vertices: cubeVertices,
    indices: cubeIndices
  });
}

//

function Enemy() {
  this.model = Model({
    geometry: new Box(),
    position: Vector3f(0, 0, -20),
    program: colorProgram,
    uniforms: {color: Vector4f(1, 0, 0, 1)},
  });
}

Enemy.prototype.update = function(idx, elapsed) {
  this.position.x = (idx * 10) - 60;
  this.position.z = elapsed - 100;
};

//

function Menu() {
  this.model = Model({
    position: Vector3f(0, 0, -8),
    text: 'Start Game',
    textColor: Vector4f(0.1, 0.1, 0.1, 1),
    textSize: 12,
    onGazeHoverOver: function(ev) {
      this.textColor.x = 1;
    },
    onGazeHoverOut: function(ev) {
      this.textColor.x = 0.1;
    },
    onGestureTouchUp: function(ev) {
      handleAction(ACTION_START_GAME);
    },
  });
}

//

function Game() {
  this.model = Model({
    onFrame: function(ev) {
      handleAction(ACTION_FRAME, ev.now);
    }
  });
}

env.scene.add(state.menu.model);

}
function vrmain(env) {

  var program = env.core.Program((
    '#version 300 es\n'+
    'in vec3 Position;\n'+
    'uniform mat4 Modelm;\n'+
    'uniform mat4 Viewm;\n'+
    'uniform mat4 Projectionm;\n'+
    'out vec4 fragmentColor;\n'+
    'void main()\n'+
    '{\n'+
    ' gl_Position = Projectionm * (Viewm * (Modelm * vec4(Position, 1.0)));\n'+
    ' fragmentColor = vec4(0, 0, 0, 1);\n'+
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

  var wrench = env.core.Model({
    program: program,
    position: env.core.Vector3f(0, 0, -5),
    scale: env.core.Vector3f(10, 10, 10),
    file: 'assets/wrench.fbx',
    onFrame: function(ev) {
      var pos = ev.viewPos.add(ev.viewFwd.multiply(5));
      // TODO: Make this not necessary
      this.position.x = pos.x;
      this.position.y = pos.y;
      this.position.z = pos.z;
    }
  });

  env.scene.add(wrench);

}
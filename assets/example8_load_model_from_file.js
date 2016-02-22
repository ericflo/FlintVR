function vrmain(env) {

  var program = Flint.Core.Program((
    '#version 300 es\n'+
    'in vec3 Position;\n'+
    'in vec2 TexCoord;\n'+
    'uniform mat4 Modelm;\n'+
    'uniform mat4 Viewm;\n'+
    'uniform mat4 Projectionm;\n'+
    'out highp vec2 oTexCoord;\n'+
    'void main()\n'+
    '{\n'+
    ' gl_Position = Projectionm * (Viewm * (Modelm * vec4(Position, 1.0)));\n'+
    ' oTexCoord = TexCoord;\n'+
    '}'
  ), (
    '#version 300 es\n'+
    'uniform sampler2D Texture0;\n'+
    'in highp vec2 oTexCoord;\n'+
    'out highp vec4 outColor;\n'+
    'void main()\n'+
    '{\n'+
    ' outColor = texture(Texture0, oTexCoord);\n'+
    '}'
  ));

  var wrench = Flint.Core.Model({
    program: program,
    position: Flint.Core.Vector3f(0, -40, -40),
    rotation: Flint.Core.Vector3f(Math.PI / 2.0 /* 90 degrees */, 0, 0),
    scale: Flint.Core.Vector3f(0.2, 0.2, 0.2),
    file: 'assets/cube.fbx',
    onFrame: function(ev) {
      var pos = ev.viewPos.add(ev.viewFwd.multiply(5));
      this.position.x = pos.x;
      this.position.y = pos.y;
      this.position.z = pos.z;
    }
  });

  Flint.scene.add(wrench);

  Flint.scene.background = Flint.Core.Texture({
    path: 'assets/CubePano.jpg',
    width: 1536,
    height: 1536,
    cube: true
  });
}
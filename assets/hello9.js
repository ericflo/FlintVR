function vrmain(env) {

  var program = env.core.Program((
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

  var wrench = env.core.Model({
    program: program,
    position: env.core.Vector3f(0, -40, -40),
    //rotation: env.core.Vector3f(Math.PI / 2.0 /* 90 degrees */, 0, 0),
    //scale: env.core.Vector3f(0.2, 0.2, 0.2),
    file: 'assets/Trident-A10-Embedded.fbx'/*,
    onFrame: function(ev) {
      var pos = ev.viewPos.add(ev.viewFwd.multiply(10));
      this.position.x = pos.x;
      this.position.y = pos.y;
      this.position.z = pos.z;
    }*/
  });

  env.scene.add(wrench);

  env.scene.background = env.core.Texture({
    path: 'assets/CubePano.jpg',
    width: 1536,
    height: 1536,
    cube: true
  });
}
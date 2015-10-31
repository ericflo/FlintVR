function vrmain(env) {
  env.scene.background = env.core.Texture({
    path: 'assets/CubePano.jpg',
    width: 1536,
    height: 1536,
    cube: true
  });
}
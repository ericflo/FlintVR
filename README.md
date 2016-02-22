FlintVR
--------

An experimental VR engine built in C++, but controlled with JS. 


Prerequisites
-------------

* GearVR
* Oculus Mobile SDK 1.0.0.0
* Configured Android development tools
* An Oculus developer signature file
* Python


Running the Example
-------------------

* Clone FlintVR into your ovr_sdk_mobile_1.0.0.0/VrSamples/Native folder
* Copy your signature file into ovr_sdk_mobile_1.0.0.0/VrSamples/Native/FlintVR/assets
* Change directory to ovr_sdk_mobile_1.0.0.0/VrSamples/Native/FlintVR/Projects/Android
* Run python build.py debug


Running the Other Examples
--------------------------

* Modify SCRIPT_PATH in Src/OvrApp.cpp to:
  * example1_cubes_and_stars.js
  * example2_model_parenting.js
  * example3_collision.js
  * example4_dynamic_uniforms.js
  * example5_textures.js
  * example6_cubemap_background.js
  * example7_text.js
  * example8_load_model_from_file.js


Loading from the Internet
-------------------------

* Modify SCRIPT_URL in Src/OvrApp.cpp to your url
* Make sure to create a flint.json (like index.html) at docroot


Example flint.json
------------------
```
{
  "title": "Intro to FlintVR App!",
  "files": [
    "build/main.js",
    "textures/texture1.jpg",
    "textures/texture2.jpg"
  ],
  "entrypoint": "build/main.js"
}
```
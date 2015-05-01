breeze 2 is a streamlined library that aims at providing a minimal amount of architecture commonly needed from content creation and resource management to the interactive simulation and rendering of complex three-dimensional environments in real-time, while retaining both high efficiency and simplicity, trying to keep dependencies and encapsulation low. The library takes data-oriented approaches in performance-critical portions such as rendering, where it ventures into a generic approach to unifying data orientation and polymorphism. The library provides a multitude of openly designed sub-systems that, due to very flat hierarchies, may easily be extended or modified, with the existent architecture serving to take over the dull and generic tasks.

## News ##

  * Preliminary [API "documentation"](http://www.alphanew.net/projects/breeze2/doc) now online

## State ##

Currently, this page is in preview mode, which means that all the source is there, openly accessible to anyone. Unfortunately, however, I have yet to find the time to upload documentation, samples and a distribution that works out of the box.

## Dependencies ##

The library makes use of the [lean](http://code.google.com/p/lean-cpp-lib) C++ library, DirectX, PhysX, [Assimp](http://assimp.sourceforge.net/) (tools), Qt (tools), [utf8-cpp](http://utfcpp.sourceforge.net/), [rapidxml](http://rapidxml.sourceforge.net/), the STL and tiny fractions of boost (header-only pointer containers library, never referenced in public interface headers).

## Compatibility ##

The library makes use of some C++0x features such as move semantics to improve efficiency, yet it tries to retain backwards-compatibility with the previous C++ standard. Currently, platform-specific components are only implemented for Microsoft Visual C++.

## Sub-systems ##

  * **Core**
    * **Generic Reflection:** Property-based reflection of object state via normal, unrestricted C++ setters & getters plus a simple static in-code description (using lean-cpp-lib property template magic).
    * **Generic Serialization:** Efficient decentral index of types and corresponding serializer objects.
    * **Generic Parameters:** Holding state throughout complex generic serialization processes.
    * **Content Management:** Generic content provider interfaces for flexible resource management, including default file system implementations incorporating memory mapping.
    * **File System:** Asynchronous file watches, resource path environments.

  * **Entity System**
    * **Entity:** Part of a world, taking part in simulations and rendering when attached via its controllers.
    * **Controller:** Keeps entity state in synch with simulation state, manages rendering data and performs rendering
    * **Simulation:** Modifies world state via its controllers.
    * **Generic serialization:** Decentral index of entity & controller types and corresponding serializer objects.

  * **Graphics**
    * **Ultra-lightweight DirectX 11 wrappers:** Allow for resource management & passing of DirectX 11 objects without inducing direct DirectX 11 dependencies. Typical wrapper has little functionality, more of a handle.
    * **Effect cache:** Compiles and caches effects (HLSL FX format), automatically re-compiling on source code updates.
    * **Texture cache:** Shared usage of texture resources.
    * **Texture target pool:** Shared usage of render targets & staging textures.

  * **Scene**
    * **Rendering pipeline:** Highly customizable, provides arbitrary pipeline stage & render queue slots to allow for layered or deferred rendering styles incorporating arbitrary intermediate processing steps.
    * **Perspective-driven rendering:** Arbitrary stages may be rendered from arbitrary perspectives to allow for natural integration of shadow & reflection maps or even GPU-readback.
    * **Data-oriented rendering:** All relevant data is stored in consecutive chunks of memory layed out in order of rendering (pre-sorted by material) and render passes (pre-sorted by stages, queues, shaders & state).
    * **Effect-driven rendering:** Considerably enhanced HLSL FX format including innumerable annotations that allow for specification of whole processing chains (definition of new render targets, automated swapping of targets) and even the entire rendering pipeline itself in shader code.

  * **Physics**
    * **PhysX integration:** Rigid bodies, static actors, character controllers
    * **Shapes:** Import of compound shapes built from 3D exchange formats such as collada

  * **Content Creation**
    * **Resource compiler:** Transforms content from 3D exchange formats such as collada to efficient & specific internal formats, e.g. static meshes or PhysX shape compounds using Assimp.
    * **breezEd:** Visual scene editor.

## Acknowledgements ##

There are quite some people who have heavily influenced the way this library has turned out, among them Chris Maiwald, who not only kindly gave broad insight into his highly optimized "Cric Framework", bearing tons of well-researched knowledge and experience, but also documented his findings in countless detailed forum posts, thus providing a valuable and evergrowing source of information on many sparsely covered topics. Also among them is Michael Kenzel, with whom I had the chance to discuss many problems and ideas, and who was never reluctant to share his expertise on manifold topics, including the C++ standard. Finally, among them is Michael Raulf, often providing me with split-second answers when it comes to C++' peculiarities, as well as bringing up important idioms whenever they could come in handy.
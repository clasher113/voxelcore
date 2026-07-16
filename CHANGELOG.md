# 0.30 - 2025.11.27

[Documentation](https://github.com/MihailRis/VoxelEngine-Cpp/tree/release-0.30/doc/en/main-page.md) for 0.30

Table of contents:

- [Added](#added)
    - [Functions](#functions)
    - [Changes](#changes)
- [Fixes](#fixes)

## Added

- audio recording
- in-memory filesystems
- `:block` placement (generator)
- `on_block_present`, `on_block_removed` events
- debugging server
- `rotate` and `origin` vcm modifiers (rotation)
- custom audio streams in lua (audio.PCMStream class)
- add I16view, I32view, U16view, I32view lua classes
- libraries:
    - compression
    - audio.input
- editing atlas textures feature
- add non_reset_packs argument to app.reset_content
- freeze debug panel values on cursor unlock
- project:content content source
- project start application script
- uinode.exists property
- uinode events:
    - `onrightclick`
    - `onmouseover`
    - `onmouseout`
- frameless window mode
- settings:
    - `display.adaptive-menu-fps` experimental flag
    - `graphics.soft-lighting`
- command-line arguments:
    - `--tps` command line argument for headless mode
    - `--dbg-server` command line argument
- added error callback argument to network.tcp_connect
- go back in menu on Escape pressed
- publish `hud.exchange-slot` element
- add `istoplevel` argument to input.add_callback
- engine pause mode (debugging)
- rebuild mip-maps on texture reload
- documentation:
    - documented player.set_camera and related
    - documented `on_hud_render` event
    - update app library docs
    - updated content-packs docs (added full pack structure)

### Changes

- optimization:
    - fixed major chunks loading performance issue
    - reduced headless mode chunks memory consumption
    - vecn functions optimization is not experimental now
- pass pack environment to menu page script
- canvas element autoresize
- disable mouse camera control if non-standard camera used
- events without prefix are forbidden now
- player.* functions now throw exception in headless mode if player id not specified

### Functions

- app.create_memory_device
- app.focus
- app.get_content_sources
- app.open_url
- app.set_content_sources
- app.start_debug_instance
- assets.to_canvas
- audio.get_all_input_devices_names
- audio.get_input_info
- audio.input.fetch
- audio.input.request_open
- canvas:add
- canvas:encode
- canvas:get_data
- canvas:mul
- canvas:rect
- canvas:sub
- canvas:unbind_texture
- Canvas.decode
- compression.decode
- compression.encode
- debug.get_pack_by_frame
- file.create_memory_device
- gui.ask
- gui.set_syntax_styles
- gui.show_message
- hud.is_open
- input.get_mouse_delta
- network.find_free_port
- PCMStream:create_sound
- PCMStream:feed
- PCMStream:share
- player.get_all
- player.get_all_in_radius
- player.get_dir
- player.get_interaction_distance
- player.get_nearest
- player.set_interaction_distance
- socket:is_nodelay
- socket:recv_async
- socket:set_nodelay
- string.escape_xml
- textbox:indexByPos
- textbox:lineY
- utf8.escape_xml

## Fixes

- [fix: "Bytearray expected, got function"](https://github.com/MihailRis/voxelcore/commit/2d1c69ee7e7248d8e86e00c4a2f5cead490cd278)
- [fix zero frames texture animation fatal error](https://github.com/MihailRis/voxelcore/commit/e9222976efa67e2fe541962aabb6485363a12354)
- [fix non-local players interpolation and head direction](https://github.com/MihailRis/voxelcore/commit/75ef603df0b1958e0c9eb95c2474dc7f3ec34057)
- [fix std::bad_alloc caused by corrupted regions](https://github.com/MihailRis/voxelcore/commit/47cdc0213723c74be3e41b39702b656a2db0448d)
- [fix 'align' ui property reading](https://github.com/MihailRis/voxelcore/commit/c8ba5b5dbb7980a6576444fed9f7ea66ae1ed32a)
- [fix incorrect textbox horizontal scroll](https://github.com/MihailRis/voxelcore/commit/bc86a3d8da4301ea6ce94c52715bd7cf863b0c37)
- [fix: wrong environment used in modules imported by require(...)](https://github.com/MihailRis/voxelcore/commit/5626163f17a252212607dc63bfcd726df44bf290)
- [fix: some container attributes not available in panel](https://github.com/MihailRis/voxelcore/commit/8a858beeb421495247a8dfae064672bcf6eb4190)
- [fix generation.load_fragment](https://github.com/MihailRis/voxelcore/commit/b4ba2da95524025991f07be87b61ecc015f12656)
- [fix: byteutil.unpack 'b' is equivalent of 'B'](https://github.com/MihailRis/voxelcore/commit/2a9507b54e58f852b558d7bc9b2cc88397d37a34)
- [fix: missing yaml null literals](https://github.com/MihailRis/voxelcore/commit/5755c616f35cc1c1fc5e94ecbc9c38a5a7f52275)
- [fix yaml array parsing](https://github.com/MihailRis/voxelcore/commit/026ae756cf4ad4a4febbef58ce2f007b2a0fc974)
- [fix mouse click textbox caret set](https://github.com/MihailRis/voxelcore/commit/a1f0c2c2527b91d3a1d4f47eb2c043ebdef60119)
- [fix wrapped textbox selection render](https://github.com/MihailRis/voxelcore/commit/76b54a890c35b6edb6d5018078c865f31e966965)
- [fix broken dependencies management](https://github.com/MihailRis/voxelcore/commit/ee6f006b797d1a560baa9e333c99ee0181f548b6)
- [fix assets.parse_model with 'xml' format](https://github.com/MihailRis/voxelcore/commit/ec94abccbc4604e5d945d2bb8d1a63561797092b)
- [fix correct line generation algorithm and bounds calculation](https://github.com/MihailRis/voxelcore/commit/ed9cf8800aea07c169719a3efddae7020e6be7e9)
- [fix enable per-variant custom model caching](https://github.com/MihailRis/voxelcore/commit/2a1d2f9354ee2ab623aca9d0059c0a676395d6d0)
- [fix: stream stops and dies on underflow](https://github.com/MihailRis/voxelcore/commit/cf561e78a81810fcb70975c7b785938af37b4b64)
- [fix Schedule:set_timeout](https://github.com/MihailRis/voxelcore/commit/1e16ab5464b80d258fd955cf0485ebf596298ab3)
- [fix major chunks loading performance issue](https://github.com/MihailRis/voxelcore/commit/957f9f59983790583fb57c9e9a3661631f380153)
- [fix: undo/redo also available when textbox is not editable](https://github.com/MihailRis/voxelcore/commit/0df5684adf3117f4018e4b34868f6c41ee7125e3)
- [fix player library docs](https://github.com/MihailRis/voxelcore/commit/51f07450d89781ce35bd630e07ea8cf33b912b93)
- [fix: root node id overriding](https://github.com/MihailRis/voxelcore/commit/91cb5ab7d8ca5376d87e1ad69b8f2cb05278d73d)
- [fix extended block placement across chunk borders](https://github.com/MihailRis/voxelcore/commit/b85c5e367a2adea57ab0c3111e3d3b0cc924322b)
- [fix modelviewer fbo creation](https://github.com/MihailRis/voxelcore/commit/be6710bc831f700bd9099860c704b3571d1d90fa)
- [fix: panel width differs to size specified in xml](https://github.com/MihailRis/voxelcore/commit/8cc51a107e27e1cb5ae8343fa0151db9c9a66852)

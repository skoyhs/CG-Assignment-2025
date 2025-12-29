target("render")
	set_kind("static")
    set_languages("c++23") -- C++23标准

    add_packages("libsdl3", "glm", "imgui")

    add_rules("asset.shader", {debug = is_mode("debug"), includedir = "shader/common"})
    add_rules("asset.pack")

    add_files("src/**.cpp")
    add_includedirs("include", {public=true})
    add_headerfiles("include/(**.hpp)")

    add_files("shader/**.frag", "shader/**.vert", "shader/**.comp")

    add_files("asset/*.pack-desc")

    add_deps(
        "lib::gpu", 
        "lib::gltf", 
        "lib::zip", 
        "lib::util", 
        "lib::backend", 
        "lib::image.io",
        "lib::image.compress",
        "lib::image.algo",
        "lib::graphics.aa",
        "lib::graphics.camera",
        "lib::graphics.geometry",
        "lib::graphics.util",
        "lib::wavefront",
        {public=true}
    )
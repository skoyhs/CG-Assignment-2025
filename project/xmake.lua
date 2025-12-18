target("main")
    set_kind("binary")
    set_languages("c++23") -- C++23标准

    add_files("src/**.cpp")
    add_includedirs("include", {public=true})
    add_headerfiles("include/(**.hpp)")

    add_deps("render")

add_rules("mode.debug", "mode.release")

target("inOneWeekend")
set_kind("binary")
add_files("src/*.cc")
set_languages("c++17")

on_run(function(target)
	os.mkdir("image")

	os.execv(target:targetfile(), {}, { stdout = "image/image.ppm" })
end)

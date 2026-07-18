"""Headless Blender render: GLB -> transparent PNG at a chosen camera angle.

Invoked as:
  blender --background --factory-startup --python render_glb.py -- \
    --glb model.glb --out out.png --size 256 --azim 0 --elev 20 [--frames 1 --azim-step 45]

Renders an orthographic, transparent-background sprite of the model, framed to fill the image.
For a single item sprite use --frames 1. For a turntable / unit directions use --frames N with
--azim-step (each frame N gets azim + N*azim-step) writing out_000.png, out_001.png, ...
Uses Cycles CPU (reliable headless) with even studio lighting.
"""

import math
import sys

import bpy
import mathutils


def argv_after_dashes():
	a = sys.argv
	return a[a.index("--") + 1:] if "--" in a else []


def parse_args(av):
	d = {"glb": None, "out": None, "size": 256, "azim": 0.0, "elev": 20.0,
	     "frames": 1, "azim_step": 45.0, "margin": 1.10, "samples": 48}
	i = 0
	while i < len(av):
		k = av[i].lstrip("-").replace("-", "_")
		if k in d and i + 1 < len(av):
			v = av[i + 1]
			cur = d[k]
			if isinstance(cur, bool):
				d[k] = v.lower() in ("1", "true", "yes")
			elif isinstance(cur, int):
				d[k] = int(float(v))
			elif isinstance(cur, float):
				d[k] = float(v)
			else:  # str or None
				d[k] = v
			i += 2
		else:
			i += 1
	return d


def clear_scene():
	bpy.ops.wm.read_factory_settings(use_empty=True)


def import_glb(path):
	bpy.ops.import_scene.gltf(filepath=path)
	meshes = [o for o in bpy.context.scene.objects if o.type == "MESH"]
	return meshes


def scene_bounds(objs):
	mn = mathutils.Vector((1e9, 1e9, 1e9))
	mx = mathutils.Vector((-1e9, -1e9, -1e9))
	for o in objs:
		for c in o.bound_box:
			w = o.matrix_world @ mathutils.Vector(c)
			for i in range(3):
				mn[i] = min(mn[i], w[i])
				mx[i] = max(mx[i], w[i])
	return mn, mx


def setup(dcfg):
	scn = bpy.context.scene
	scn.render.engine = "CYCLES"
	scn.cycles.device = "CPU"
	scn.cycles.samples = int(dcfg["samples"])
	scn.render.film_transparent = True
	scn.render.resolution_x = int(dcfg["size"])
	scn.render.resolution_y = int(dcfg["size"])
	scn.render.image_settings.file_format = "PNG"
	scn.render.image_settings.color_mode = "RGBA"

	# bright, even studio lighting: strong world ambient + key + fill suns
	world = bpy.data.worlds.new("W")
	scn.world = world
	world.use_nodes = True
	bg = world.node_tree.nodes.get("Background")
	if bg:
		bg.inputs[1].default_value = 1.1  # ambient strength (bright, even base)
	key = bpy.data.lights.new("Key", "SUN")
	key.energy = 3.5
	ko = bpy.data.objects.new("Key", key)
	scn.collection.objects.link(ko)
	ko.rotation_euler = (math.radians(50), 0, math.radians(35))
	fill = bpy.data.lights.new("Fill", "SUN")
	fill.energy = 1.5
	fo = bpy.data.objects.new("Fill", fill)
	scn.collection.objects.link(fo)
	fo.rotation_euler = (math.radians(60), 0, math.radians(-140))


def place_camera(center, radius, azim_deg, elev_deg, margin):
	az = math.radians(azim_deg)
	el = math.radians(elev_deg)
	dist = radius * 3.0 + 1.0
	cam_data = bpy.data.cameras.new("Cam")
	cam_data.type = "ORTHO"
	cam_data.ortho_scale = radius * 2.0 * margin
	cam = bpy.data.objects.new("Cam", cam_data)
	bpy.context.scene.collection.objects.link(cam)
	bpy.context.scene.camera = cam
	pos = mathutils.Vector((
		center.x + dist * math.cos(el) * math.sin(az),
		center.y - dist * math.cos(el) * math.cos(az),
		center.z + dist * math.sin(el),
	))
	cam.location = pos
	direction = (center - pos).normalized()
	cam.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()
	return cam


def main():
	cfg = parse_args(argv_after_dashes())
	if not cfg["glb"] or not cfg["out"]:
		print("RENDER_ERROR missing --glb/--out")
		sys.exit(2)
	clear_scene()
	meshes = import_glb(cfg["glb"])
	if not meshes:
		print("RENDER_ERROR no meshes in glb")
		sys.exit(3)
	mn, mx = scene_bounds(meshes)
	center = (mn + mx) * 0.5
	radius = max((mx - mn).length * 0.5, 0.001)
	setup(cfg)

	frames = int(cfg["frames"])
	for f in range(frames):
		az = cfg["azim"] + f * cfg["azim_step"]
		# remove any prior camera
		for o in [o for o in bpy.context.scene.objects if o.type == "CAMERA"]:
			bpy.data.objects.remove(o, do_unlink=True)
		place_camera(center, radius, az, cfg["elev"], cfg["margin"])
		out = cfg["out"] if frames == 1 else cfg["out"].replace(".png", f"_{f:03d}.png")
		bpy.context.scene.render.filepath = out
		bpy.ops.render.render(write_still=True)
		print(f"RENDER_OK {out}")


main()

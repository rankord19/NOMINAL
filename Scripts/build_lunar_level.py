"""Run with UnrealEditor-Cmd -run=pythonscript -script=<absolute path>.
Creates only /Game/LunarPrototype assets; requires prepared inputs in Saved/LunarSources.
"""
import unreal as u
from pathlib import Path

ROOT=Path(u.Paths.project_dir())
SRC=ROOT/'Saved/LunarSources'
DEST='/Game/LunarPrototype'
assets=u.AssetToolsHelpers.get_asset_tools()
edit=u.EditorAssetLibrary
matlib=u.MaterialEditingLibrary

def import_asset(name, ext):
    task=u.AssetImportTask()
    task.filename=str(SRC/(name+ext))
    task.destination_path=DEST
    task.destination_name=name
    task.automated=True
    task.replace_existing=True
    task.save=True
    if ext=='.obj':
        task.factory=u.FbxFactory()
        options=u.FbxImportUI()
        options.import_mesh=True
        options.import_materials=False
        options.import_textures=False
        options.import_as_skeletal=False
        options.mesh_type_to_import=u.FBXImportType.FBXIT_STATIC_MESH
        options.static_mesh_import_data.combine_meshes=True
        options.static_mesh_import_data.convert_scene=False
        options.static_mesh_import_data.generate_lightmap_u_vs=False
        task.options=options
    assets.import_asset_tasks([task])
    obj=edit.load_asset(DEST+'/'+name)
    if not obj: raise RuntimeError('Import failed: '+name)
    return obj

textures={name:import_asset(name,'.png') for name in ['T_MoonColor','T_MoonNormal','T_SolarCells','T_FoilNormal']}
for name,tex in textures.items():
    if 'Normal' in name:
        tex.set_editor_property('srgb',False)
        tex.set_editor_property('compression_settings',u.TextureCompressionSettings.TC_NORMALMAP)
    if 'Moon' in name:
        tex.set_editor_property('max_texture_size',8192)
    edit.save_loaded_asset(tex)
moonmesh=import_asset('SM_Moon','.obj')
dishmesh=import_asset('SM_Dish','.obj')

def material(name, color, metal=0, rough=.7, texture=None, normal=None, two=False):
    path=DEST+'/'+name
    m=edit.load_asset(path) if edit.does_asset_exist(path) else assets.create_asset(name,DEST,u.Material,u.MaterialFactoryNew())
    matlib.delete_all_material_expressions(m)
    m.set_editor_property('two_sided',two)
    def scalar(value,prop):
        e=matlib.create_material_expression(m,u.MaterialExpressionConstant)
        e.r=value
        matlib.connect_material_property(e,'',prop)
    e=matlib.create_material_expression(m,u.MaterialExpressionConstant3Vector)
    e.constant=u.LinearColor(*color,1)
    matlib.connect_material_property(e,'',u.MaterialProperty.MP_BASE_COLOR)
    scalar(metal,u.MaterialProperty.MP_METALLIC)
    scalar(rough,u.MaterialProperty.MP_ROUGHNESS)
    if texture:
        e=matlib.create_material_expression(m,u.MaterialExpressionTextureSample)
        e.texture=textures[texture]
        matlib.connect_material_property(e,'RGB',u.MaterialProperty.MP_BASE_COLOR)
    if normal:
        e=matlib.create_material_expression(m,u.MaterialExpressionTextureSample)
        e.texture=textures[normal]
        e.sampler_type=u.MaterialSamplerType.SAMPLERTYPE_NORMAL
        matlib.connect_material_property(e,'RGB',u.MaterialProperty.MP_NORMAL)
    matlib.recompile_material(m)
    edit.save_loaded_asset(m)
    return m

mmoon=material('M_Moon',(.15,.15,.15),0,1,'T_MoonColor','T_MoonNormal',two=True)
gold=material('M_ThermalFoil',(.65,.34,.07),.65,.55,normal='T_FoilNormal')
silver=material('M_Aluminum',(.55,.58,.62),.75,.36,two=True)
white=material('M_Radiator',(.65,.68,.7),.05,.6,two=True)
black=material('M_Graphite',(.025,.03,.035),.4,.5)
solar=material('M_SolarCells',(.02,.04,.09),.45,.27,'T_SolarCells')

level=u.get_editor_subsystem(u.LevelEditorSubsystem)
actors=u.get_editor_subsystem(u.EditorActorSubsystem)
if edit.does_asset_exist(DEST+'/L_LunarOrbit'):
    if not level.load_level(DEST+'/L_LunarOrbit'): raise RuntimeError('Cannot load lunar level')
    # This generator owns this dedicated level. Rebuild only its presentation actors.
    for a in actors.get_all_level_actors():
        if isinstance(a,(u.StaticMeshActor,u.DirectionalLight,u.PostProcessVolume,u.PlayerStart)):
            actors.destroy_actor(a)
elif not level.new_level(DEST+'/L_LunarOrbit'):
    raise RuntimeError('Cannot create lunar level')

def part(label, mesh, loc, scale, mat, rot=(0,0,0)):
    a=actors.spawn_actor_from_class(u.StaticMeshActor,u.Vector(*loc),u.Rotator(pitch=rot[0],yaw=rot[1],roll=rot[2]))
    a.set_actor_label(label)
    c=a.static_mesh_component
    c.set_mobility(u.ComponentMobility.MOVABLE)
    c.set_static_mesh(edit.load_asset(mesh) if isinstance(mesh,str) else mesh)
    c.set_material(0,mat)
    c.set_collision_enabled(u.CollisionEnabled.NO_COLLISION)
    a.set_actor_scale3d(u.Vector(*scale))
    return a

cube='/Engine/BasicShapes/Cube'
cyl='/Engine/BasicShapes/Cylinder'
cone='/Engine/BasicShapes/Cone'
moon=part('Moon_LOLA_ReferenceRadius_1737_4km',moonmesh,(0,0,-183740000),(1737400,)*3,mmoon, (0,0,90))
moon.static_mesh_component.set_editor_property('cast_shadow',False)
# Model origin is the camera's spacecraft focus. All dimensions here are visual placeholders.
part('Orbiter_ThermalBus',cube,(0,0,0),(1.8,1.5,2),gold)
for z in (-105,105): part('Bus_Deck',cube,(0,0,z),(2,1.7,.1),silver)
for x in (-93,93):
    for y in (-78,78): part('Bus_EdgeRail',cube,(x,y,0),(.06,.06,2.15),silver)
part('Radiator',cube,(0,-77,10),(1.3,.035,1.4),white)
for z in range(-50,80,14): part('Radiator_Seam',cube,(0,-80,z),(1.3,.02,.012),black)
part('Antenna_Mast',cyl,(0,0,160),(.12,.12,1),silver)
part('HighGain_Dish_TEMP',dishmesh,(0,0,210),(1,1,1),white, (-35,-58,0))
part('Antenna_Feed',cyl,(0,0,240),(.06,.06,.5),silver)
part('Engine_Nozzle_TEMP',cone,(0,0,-145),(.65,.65,.7),black)
part('Engine_Mount',cyl,(0,0,-112),(.5,.5,.15),silver)
for side in (-1,1):
    part('Array_Boom',cube,(side*160,0,0),(1.3,.08,.08),silver)
    for section in range(3):
        x=side*(240+section*125)
        part('Array_Frame',cube,(x,0,0),(1.22,2.25,.065),silver)
        part('Solar_Cells_TEMP',cube,(x,0,4),(1.16,2.19,.025),solar)
    for y in (-65,65):
        part('RCS_Nozzle_TEMP',cone,(side*100,y,-80),(.17,.17,.28),black)

sun=actors.spawn_actor_from_class(u.DirectionalLight,u.Vector(0,0,1000),u.Rotator(pitch=-35,yaw=122,roll=0))
sun.set_actor_label('Sun_Directional_NoAtmosphere')
sun.light_component.set_editor_property('mobility',u.ComponentMobility.MOVABLE)
sun.light_component.set_editor_property('intensity',100000.0)
sun.light_component.set_editor_property('light_source_angle',.53)
sun.light_component.set_editor_property('cast_shadows',True)
sun.set_actor_rotation(u.Rotator(pitch=-25,yaw=122,roll=0),False)
pp=actors.spawn_actor_from_class(u.PostProcessVolume,u.Vector())
pp.set_editor_property('unbound',True)
settings=pp.get_editor_property('settings')
for prop,value in [('auto_exposure_min_brightness',14.5),('auto_exposure_max_brightness',14.5),('auto_exposure_bias',0.0),('bloom_intensity',.05),('vignette_intensity',0.0),('motion_blur_amount',0.0)]:
    settings.set_editor_property('override_'+prop,True)
    settings.set_editor_property(prop,value)
settings.set_editor_property('override_dynamic_global_illumination_method',True)
settings.set_editor_property('dynamic_global_illumination_method',u.DynamicGlobalIlluminationMethod.NONE)
pp.set_editor_property('settings',settings)
start=actors.spawn_actor_from_class(u.PlayerStart,u.Vector(1161,-1859,192),u.Rotator(pitch=-5,yaw=122,roll=0))
start.set_actor_label('OrbiterCameraStart')
# Preserve designer bindings if this optional full scene generator is run in the future.
import sys
sys.path.insert(0, str(ROOT/'Scripts'))
from tag_spacecraft_parts import tag_part
for actor in actors.get_all_level_actors():
    tag_part(actor)
level.save_current_level()
edit.save_directory(DEST,only_if_is_dirty=False,recursive=True)
u.log('NOMINAL_LUNAR_LEVEL_BUILD_SUCCESS')

"""Add runtime designer role tags only; preserve all actors, geometry and scene settings.
Run once via UnrealEditor-Cmd -run=pythonscript -script=<this file>.
"""
import unreal as u

def tag_part(actor):
    label = actor.get_actor_label()
    roles = {
        'Array_Frame': 'Designer.Panel', 'Solar_Cells_TEMP': 'Designer.Panel',
        'HighGain_Dish_TEMP': 'Designer.Dish', 'Antenna_Feed': 'Designer.Feed',
        'Engine_Nozzle_TEMP': 'Designer.Nozzle', 'Engine_Mount': 'Designer.Mount',
    }
    for prefix, role in roles.items():
        if label.startswith(prefix):
            tags = list(actor.tags)
            if u.Name(role) not in tags:
                tags.append(u.Name(role))
                actor.set_editor_property('tags', tags)
            return role
    return None

if __name__ == '__main__':
    level = u.get_editor_subsystem(u.LevelEditorSubsystem)
    assert level.load_level('/Game/LunarPrototype/L_LunarOrbit')
    counts = {}
    for actor in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors():
        role = tag_part(actor)
        if role:
            counts[role] = counts.get(role, 0) + 1
    assert counts == {'Designer.Panel': 12, 'Designer.Dish': 1, 'Designer.Feed': 1,
                      'Designer.Nozzle': 1, 'Designer.Mount': 1}, counts
    assert level.save_current_level()
    u.log('NOMINAL_DESIGNER_TAGS_SUCCESS ' + str(counts))

$cd "F:\Dev\HL_Modding\VS2013\template_mp\modelsrc\player\blue\Shared"
$modelname "player\vs_vamp_shared.mdl"
$poseparameter body_pitch -45.00 45.00
$poseparameter body_yaw -45.00 45.00
$poseparameter body_height 28.00 58.00
$poseparameter move_y -1.00 1.00
$poseparameter move_x -1.00 1.00
$cdmaterials "models\player\animation/"

$hboxset "default"
$hbox 0 "ValveBiped.Bip01_Pelvis" -3.890  -0.000  -2.982  3.890  3.345  0.000
$hbox 0 "ValveBiped.Bip01_Spine4" 0.000  -0.000  -1.938  3.307  1.001  1.938
$hbox 0 "ValveBiped.Bip01_L_Hand" 0.000  -2.717  -1.192  8.387  0.047  47.381
$hbox 0 "ValveBiped.Bip01_R_Hand" 0.000  -2.717  -1.331  8.387  0.050  47.381
$attachment "weapon_bone" "ValveBiped.weapon_bone" 0.00 0.00 -0.00 rotate -0.00 -0.00 -0.00
$attachment "weapon_bone_left" "ValveBiped.weapon_bone_left" 0.00 0.00 0.00 rotate 0.00 0.00 -0.00
$attachment "anim_attachment_RH" "ValveBiped.Bip01_R_Hand" 0.00 0.00 -0.00 rotate 0.00 -0.00 -0.00
$surfaceprop "default"
$illumposition 25.810 12.146 30.691\
$sequence ragdollspawn "ragdollspawn" fps 30.00

$sequence StandIdle_CLAWS "anims/Stand_Idle_CLAWS" loop ACT_VS_STAND_IDLE_CLAWS 1 fps 10.00 
//$sequence armBend "anims/phymodelAction" fps 30.00
//$sequence KneeRaise "anims/knee_raise" fps 30.00
//$sequence Run_Claws "anims/a_run" loop ACT_VS_RUN_CLAWS 1 fps 30.000000
$sequence Long_Jump "anims/LongJump" ACT_VS_LONG_JUMP 1 fps 30.000000

$sequence Attack_CLAWS "anims/SwingClaws" loop ACT_VS_ATTACK_CLAWS 1 fps 30.000000 subtract StandIdle_CLAWS 0 delta



$ikchain rhand ValveBiped.Bip01_R_Hand knee  0.707 0.707 0.000
$ikchain lhand ValveBiped.Bip01_L_Hand knee  0.707 0.707 0.000
$ikchain rfoot ValveBiped.Bip01_R_Foot knee  0.707 -0.707 0.000
$ikchain lfoot ValveBiped.Bip01_L_Foot knee  0.707 -0.707 0.000
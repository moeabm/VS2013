$cd "F:\Dev\HL_Modding\VS2013\template_mp\modelsrc\player\blue\Shared"
$modelname "player\vs_vamp_shared.mdl"
$poseparameter body_pitch -45.00 45.00
$poseparameter body_yaw -45.00 45.00
$poseparameter body_height 28.00 58.00
$poseparameter move_y -1.00 1.00
$poseparameter move_x -1.00 1.00
$cdmaterials "models\player\animation/"

$hboxset "default"
$hbox 0 "ValveBiped.Bip01_Pelvis" -7.750  -3.000  -6.600  7.750  3.000  4.900
$hbox 0 "ValveBiped.Bip01_L_Thigh" 3.500  -3.900  -3.900  19.500  3.100  3.100
$hbox 0 "ValveBiped.Bip01_L_Calf" 1.520  -3.370  -3.250  16.510  3.630  3.250
$hbox 0 "ValveBiped.Bip01_L_Foot" -1.000  -1.300  -2.000  5.000  2.700  2.000
$hbox 0 "ValveBiped.Bip01_R_Thigh" 3.500  -3.900  -3.100  19.500  3.100  3.900
$hbox 0 "ValveBiped.Bip01_R_Calf" 1.520  -3.370  -3.250  16.510  3.630  3.250
$hbox 0 "ValveBiped.Bip01_R_Foot" -1.000  -1.300  -2.000  5.000  2.700  2.000
$hbox 0 "ValveBiped.Bip01_Spine" -0.450  -3.000  -7.000  4.050  7.000  7.000
$hbox 0 "ValveBiped.Bip01_Spine1" -0.900  -3.000  -7.000  4.100  8.000  7.000
$hbox 0 "ValveBiped.Bip01_Spine2" -0.500  -3.900  -7.500  7.500  7.100  7.500
$hbox 0 "ValveBiped.Bip01_Spine4" -0.200  -2.800  -7.500  3.800  5.200  7.500
$hbox 0 "ValveBiped.Bip01_L_UpperArm" -3.000  -2.050  -2.450  13.000  2.450  2.050
$hbox 0 "ValveBiped.Bip01_L_Forearm" -0.400  -2.000  -2.800  11.200  2.200  1.800
$hbox 0 "ValveBiped.Bip01_L_Hand" 0.000  -1.650  -1.450  5.000  0.350  2.050
$hbox 0 "ValveBiped.Bip01_R_UpperArm" -3.000  -2.050  -2.050  13.000  2.450  2.450
$hbox 0 "ValveBiped.Bip01_R_Forearm" -0.400  -1.900  -1.800  11.200  2.100  2.400
$hbox 0 "ValveBiped.Bip01_R_Hand" 0.000  -1.650  -2.050  5.000  0.350  1.450
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
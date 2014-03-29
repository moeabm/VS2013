$cd "F:\Dev\HL_Modding\VS2013\template_mp\modelsrc\weapons\thunder5"
$modelname "weapons\v_thunder5.mdl"
$model "studio" "thunder5_reference.smd"
$model "hands" "hands_reference.smd"
$cdmaterials "models\weapons\V_models\thunder5\"
$cdmaterials "models\weapons\v_models\hands\"
$hboxset "default"
$hbox 0 "v_weapon" -0.057  0.000  -3.095  2.725  2.406  16.456
$hbox 0 "v_weapon.Thunder_Parent" -0.402  -1.775  -1.494  3.638  1.715  0.913
$hbox 0 "v_weapon.Hands_parent" -4.654  -1.415  0.000  6.834  1.302  6.902
$hbox 0 "v_weapon.Left_Arm" -0.319  -1.833  -1.434  8.563  1.491  1.708
$hbox 0 "v_weapon.Left_Hand" -0.998  -1.856  -1.460  3.655  1.901  1.020
$hbox 0 "v_weapon.Left_Thumb01" -0.038  -0.957  -1.026  2.195  0.816  0.689
$hbox 0 "v_weapon.Left_Thumb_02" -0.557  -0.812  -0.925  1.705  0.536  0.541
$hbox 0 "v_weapon.Left_Pinky01" -0.563  -0.567  -0.680  1.253  1.426  0.518
$hbox 0 "v_weapon.Left_Ring01" -0.051  -0.398  -0.616  1.498  0.636  0.413
$hbox 0 "v_weapon.L_wrist_helper" -0.805  -6.918  -1.161  1.745  0.000  2.128
$hbox 0 "v_weapon.Right_Arm" -0.402  -1.437  -1.576  8.544  1.387  1.609
$hbox 0 "v_weapon.Right_Hand" -0.402  -1.775  -1.494  3.638  1.715  0.913
$hbox 0 "v_weapon.Right_Thumb01" -0.867  -1.013  -1.300  2.356  1.051  0.852
$hbox 0 "v_weapon.Right_Thumb02" -1.285  -0.580  -0.979  1.751  0.994  0.680
$hbox 0 "v_weapon.Right_Middle01" -0.061  -0.689  -0.475  1.376  0.379  0.545
$hbox 0 "v_weapon.Right_Ring01" -0.256  -0.648  -0.542  1.312  0.423  0.473
$hbox 0 "v_weapon.Right_Pinky01" -0.456  -1.085  -0.539  1.124  0.488  0.691
$hbox 0 "v_weapon.R_wrist_helper" -1.175  -7.124  -1.587  1.829  0.933  2.029

$attachment "1" "v_weapon.Thunder_Parent" 0.00 3.00 -5.00 rotate 90.00 0.00 0.00
$attachment "muzzle" "v_weapon.Thunder_Parent" 0.00 3.00 -5.00 rotate 90.00 0.00 0.00

$surfaceprop "flesh"
$illumposition 6.714 3.706 -8.115
$sequence idle "anims\idle" ACT_VM_IDLE 1 fps 12.00
$sequence idle02 "anims\midFinger" ACT_VM_FIDGET 1 fps 16.00
$sequence shoot1 "anims\shoot1" ACT_VM_PRIMARYATTACK 1 fadeout 0.0 fps 32.00 {
  { event AE_MUZZLEFLASH 0 "PISTOL MUZZLE" }
  { event 5001 0 "1" }
}

$sequence insert "anims\reload" ACT_VM_RELOAD 1 fadeout 0.0 fps 32.00

$sequence after_reload "anims\after_reload" ACT_SHOTGUN_RELOAD_FINISH 1 fps 16.00
 
$sequence start_reload "anims\start_reload" ACT_SHOTGUN_RELOAD_START 1 fps 8.00

// $sequence attack1 "anims\attack1" ACT_VM_HITCENTER  1 fps 32.00 {
   // { event 5001 0 "1" }
 // }

// $sequence attackMiss1 "anims\attack1" ACT_VM_MISSCENTER  1 fps 32.00 {
   // { event 5001 0 "1" }
 // }
 
// $sequence attackMiss2 "anims\attack2" ACT_VM_MISSCENTER  1 fps 32.00 {
   // { event 5001 0 "1" }
 // }
 
// $sequence attackMiss3 "anims\attack3" ACT_VM_MISSCENTER  1 fps 32.00 {
   // { event 5001 0 "1" }
 // }
 
// $sequence shoot2 "shoot2" ACT_VM_PRIMARYATTACK 1 fps 37.00 {
  // { event 5001 0 "1" }
  // { event AE_CLIENT_EFFECT_ATTACH 23 "EjectBrass_12Gauge 2 70" }
  // { event 5001 0 "1" }
// }

// $sequence insert "insert" ACT_VM_RELOAD 1 fps 55.00 {
  // { event 5004 0 "Weapon_M3.Insertshell" }
// }

// $sequence after_reload "after_reload" ACT_SHOTGUN_RELOAD_FINISH 1 fps 40.00 {
  // { event 5004 12 "Weapon_M3.Pump" }
// }

// $sequence start_reload "start_reload" ACT_SHOTGUN_RELOAD_START 1 fps 40.00
// $sequence draw "draw" ACT_VM_DRAW 1 fps 30.00 {
  // { event 5004 11 "Weapon_M3.Pump" }
// }


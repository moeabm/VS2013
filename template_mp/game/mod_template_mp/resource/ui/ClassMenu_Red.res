"Resource/UI/ClassMenu_Red.res"
{
	"class_red"
	{
		"ControlName"		"CSDKClassMenu"
		"fieldName"		"class_red"
		"xpos"			"0"
		"ypos"			"0"
		"wide"			"500"
		"tall"			"400"
		"autoResize"		"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"tabPosition"		"0"
		"settitlebarvisible"	"0"
		"font"			"#Frame_Untitled"
	}
	//Tony; default class info panel that we are not using, hide it.
	"ClassInfo"
	{
		"ControlName"		"Panel"
		"fieldName"		"ClassInfo"
		"xpos"			"-10"
		"ypos"			"-10"
		"wide"			"0"
		"tall"			"0"
		"autoResize"		"3"
		"pinCorner"		"0"
		"visible"		"0"
		"enabled"		"0"
		"tabPosition"		"0"
	}
	"ClassInfoPanel"
	{
		"ControlName"		"CSDKClassInfoPanel"
		"fieldName"		"ClassInfoPanel"
		"xpos"			"5"
		"ypos"			"249"
		"wide"			"500"
		"tall"			"150"
		"autoResize"		"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"tabPosition"		"0"
		"font"			"ClassMenuDefault"
	}
	"ClassInfoBackground"
	{
		"ControlName"		"ImagePanel"
		"fieldName"		"ClassInfoBackground"
		"xpos"			"5"
		"ypos"			"249"
		"wide"			"500"
		"tall"			"150"
		"autoResize"		"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1" 
		"tabPosition"		"0"
		"fillcolor"		"ClassInfoBackground"
		"labelText"		""
	}
	"SysMenu"
	{
		"ControlName"		"Menu"
		"fieldName"		"SysMenu"
		"xpos"			"0"
		"ypos"			"0"
		"zpos"			"1"
		"wide"			"64"
		"tall"			"24"
		"autoResize"		"0"
		"pinCorner"		"0"
		"visible"		"0"
		"enabled"		"0"
		"tabPosition"		"0"
		"font"			"ClassMenuDefault"
	}
	"selectClass"
	{	
		"ControlName"		"Label"
		"fieldName"		"selectClass"
		"font"			"MenuLargeFont"
		"xpos"			"10"
		"ypos"			"10"
		"wide"			"340"
		"tall"			"26"
		"autoResize"		"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"labelText"		"#SDK_ClassMenu_SelectClass"
		"textAlignment"		"west"
		"dulltext"		"0"
		"brighttext"		"0"
	}
	"red_class1"
	{
		"ControlName"		"SDKButton"
		"fieldName"		"red_class1"
		"xpos"			"25"
		"ypos"			"44"
		"zpos"			"1"
		"wide"			"62"
		"tall"			"124"
		"autoResize"		"0"
		"pinCorner"		"2"
		"visible"		"1"
		"enabled"		"1"
		"tabPosition"		"0"
		"labelText"		"&1"
		"textAlignment"		"south-west"
		"dulltext"		"0"
		"brighttext"		"1"
		"Command"		"cls_red_class1"
		"Default"		"0"
		"font"			"DefaultLarge"
		"scaleImage"		"1"
		"activeimage"		"cls_red_class1_active"
		"inactiveimage"		"cls_red_class1"
	}
	"red_class1_model"
	{
		"ControlName"		"CModelPanel"
		"fieldName"		"red_class1_model"
		"xpos"			"25"
		"ypos"			"44"
		"zpos"			"-5"		
		"wide"			"62"
		"tall"			"124"
		"autoResize"		"0"
		"pinCorner"		"2"
		"visible"		"1"
		"enabled"		"1"
		"fov"			"50"

		"model"
		{
			"spotlight"	"1"
			"modelname"	"models/player/red_player.mdl"
			"origin_z"	"-45"
			"origin_y"	"0"
			"angles_y" "160"

			"animation"
			{
				"sequence"		"w_WalkIdle_PISTOL"
				"pose_parameters"
				{
					"move_x" "1.0"
				}
			}
			
			//"attached_model"
			//{
			//	"modelname" "models/weapons/w_pist_deagle.mdl"
			//}
		}

	}
	"red_class2"
	{
		"ControlName"		"SDKButton"
		"fieldName"		"red_class2"
		"xpos"			"138"
		"ypos"			"44"
		"wide"			"62"
		"tall"			"124"
		"autoResize"		"0"
		"pinCorner"		"2"
		"visible"		"1"
		"enabled"		"1"
		"tabPosition"		"0"
		"labelText"		"&2"
		"textAlignment"		"south-west"
		"dulltext"		"0"
		"brighttext"		"1"
		"Command"		"cls_red_class2"
		"Default"		"0"
		"font"			"DefaultLarge"
		"scaleImage"		"1"
		"activeimage"		"cls_red_class2_active"
		"inactiveimage"		"cls_red_class2"
	}
	"red_class2_model"
	{
		"ControlName"		"CModelPanel"
		"fieldName"		"red_class2_model"
		"xpos"			"138"
		"ypos"			"44"
		"zpos"			"-5"		
		"wide"			"62"
		"tall"			"124"
		"autoResize"		"0"
		"pinCorner"		"2"
		"visible"		"1"
		"enabled"		"1"
		"fov"			"50"

		"model"
		{
			"spotlight"	"1"
			"modelname"	"models/player/red_player.mdl"
			"origin_z"	"-45"
			"origin_y"	"0"
			"angles_y" "180"

			"animation"
			{
				"sequence"		"w_WalkIdle_RIFLE"
				"pose_parameters"
				{
					"move_x" "1.0"
				}
			}
			
			//"attached_model"
			//{
			//	"modelname" "models/weapons/w_shot_m3super90.mdl"
			//}
		}

	}
	"red_class3"
	{
		"ControlName"		"SDKButton"
		"fieldName"		"red_class3"
		"xpos"			"252"
		"ypos"			"44"
		"wide"			"62"
		"tall"			"124"
		"autoResize"		"0"
		"pinCorner"		"2"
		"visible"		"1"
		"enabled"		"1"
		"tabPosition"		"0"
		"labelText"		"&3"
		"textAlignment"		"south-west"
		"dulltext"		"0"
		"brighttext"		"1"
		"Command"		"cls_red_class3"
		"Default"		"0"
		"font"			"DefaultLarge"
		"scaleImage"		"1"
	}
	"red_class3_model"
	{
		"ControlName"		"CModelPanel"
		"fieldName"		"red_class3_model"
		"xpos"			"252"
		"ypos"			"44"
		"zpos"			"-5"		
		"wide"			"62"
		"tall"			"124"
		"autoResize"		"0"
		"pinCorner"		"2"
		"visible"		"1"
		"enabled"		"1"
		"fov"			"50"

		"model"
		{
			"spotlight"	"1"
			"modelname"	"models/player/red_player.mdl"
			"origin_z"	"-45"
			"origin_y"	"10"
			"angles_y" "220"

			"animation"
			{
				"sequence"		"w_WalkIdle_TOMMY"
				"pose_parameters"
				{
					"move_x" "1.0"
				}
			}
			
			//"attached_model"
			//{
			//	"modelname" "models/weapons/w_smg_mp5.mdl"
			//}
		}

	}
	"CancelButton"
	{
		"ControlName"		"SDKButton"
		"fieldName"		"CancelButton"
		"xpos"			"337"
		"ypos"			"189"
		"wide"			"110"
		"tall"			"16"
		"autoResize"		"0"
		"pinCorner"		"2"
		"visible"		"0"
		"enabled"		"1"
		"tabPosition"		"0"
		"labelText"		"#SDK_Cancel"
		"textAlignment"		"center"
		"dulltext"		"0"
		"brighttext"		"0"
		"font"			"ClassMenuDefault"
		"wrap"			"0"
		"Command"		"vguicancel"
		"Default"		"0"
	}
	"random"
	{
		"ControlName"		"SDKButton"
		"fieldName"		"random"
		"xpos"			"366"
		"ypos"			"44"
		"wide"			"62"
		"tall"			"124"
		"autoResize"		"0"
		"pinCorner"		"2"
		"visible"		"1"
		"enabled"		"1"
		"tabPosition"		"0"
		"labelText"		"#SDK_ClassMenu_Random"
		"textAlignment"		"center"
		"font"			"ClassMenuDefault"
		"dulltext"		"0"
		"brighttext"		"0"
		"Command"		"cls_random"
		"Default"		"0"
	}
	"selectedClassLabel"
	{
		"ControlName"		"Label"
		"fieldName"		"selectedClassLabel "
		"xpos"			"25"
		"ypos"			"227"
		"wide"			"170"
		"tall"			"20"
		"autoResize"		"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"tabPosition"		"0"
		"labelText"		"#SDK_ClassMenu_SelectedClass"
		"textAlignment"		"west"
		"dulltext"		"1"
		"brighttext"		"0"
		"font"			"ClassMenuBodyFont"
		"wrap"			"0"
	}
	"class_1_icon"
	{
		"ControlName"		"CIconPanel"
		"fieldName"		"class_1_icon"
		"xpos"			"28"
		"ypos"			"186"
		"wide"			"16"
		"tall"			"25"
		"visible"		"1"
		"enabled"		"1"
		"scaleImage"		"1"	
		"icon"			"capture_icon"
		"iconColor"		"Label.TextDullColor"
	}
	
	"class_1_num"
	{
		"ControlName"		"Label"
		"fieldName"		"class_1_num"
		"xpos"			"44"
		"ypos"			"191" 
		"wide"			"45"
		"tall"			"20"
		"autoResize"		"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"tabPosition"		"0"
		"labelText"		"asdf"
		"textAlignment"		"west"
		"dulltext"		"1"
		"brighttext"		"0"
		"font"			"Default"
		"wrap"			"0"
	}
	
	"class_2_icon"
	{
		"ControlName"		"CIconPanel"
		"fieldName"		"class_2_icon"
		"xpos"			"142"
		"ypos"			"186"
		"wide"			"16"
		"tall"			"25"
		"visible"		"1"
		"enabled"		"1"
		"scaleImage"		"1"	
		"icon"			"capture_icon"
		"iconColor"		"Label.TextDullColor"
	}
	
	"class_2_num"
	{
		"ControlName"		"Label"
		"fieldName"		"class_2_num"
		"xpos"			"158"
		"ypos"			"191" 
		"wide"			"62"
		"tall"			"20"
		"autoResize"		"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"tabPosition"		"0"
		"labelText"		"asdf"
		"textAlignment"		"west"
		"dulltext"		"1"
		"brighttext"		"0"
		"font"			"Default"
		"wrap"			"0"
	}
		
	"class_3_icon"
	{
		"ControlName"		"CIconPanel"
		"fieldName"		"class_3_icon"
		"xpos"			"255"
		"ypos"			"186"
		"wide"			"16"
		"tall"			"25"
		"visible"		"1"
		"enabled"		"1"
		"scaleImage"		"1"	
		"icon"			"capture_icon"
		"iconColor"		"Label.TextDullColor"
	}
	
	"class_3_num"
	{
		"ControlName"		"Label"
		"fieldName"		"class_3_num"
		"xpos"			"271"
		"ypos"			"191" 
		"wide"			"62"
		"tall"			"20"
		"autoResize"		"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"tabPosition"		"0"
		"labelText"		"asdf"
		"textAlignment"		"west"
		"dulltext"		"1"
		"brighttext"		"0"
		"font"			"Default"
		"wrap"			"0"
	}	
	"class_1_full"
	{
		"ControlName"		"Label"
		"fieldName"		"class_1_full"
		"xpos"			"25"
		"ypos"			"213" 
		"wide"			"62"
		"tall"			"20"
		"autoResize"		"0"
		"pinCorner"		"0"
		"visible"		"0"
		"enabled"		"1"
		"tabPosition"		"0"
		"labelText"		"#SDK_ClassMenu_Full"
		"textAlignment"		"center"
		"dulltext"		"0"
		"brighttext"		"1"
		"font"			"ClassMenuBodyFont"
		"wrap"			"0"
	}
	
	"class_2_full"
	{
		"ControlName"		"Label"
		"fieldName"		"class_3_full"
		"xpos"			"138"
		"ypos"			"213" 
		"wide"			"62"
		"tall"			"20"
		"autoResize"		"0"
		"pinCorner"		"0"
		"visible"		"0"
		"enabled"		"1"
		"tabPosition"		"0"
		"labelText"		"#SDK_ClassMenu_Full"
		"textAlignment"		"center"
		"dulltext"		"0"
		"brighttext"		"1"
		"font"			"ClassMenuBodyFont"
		"wrap"			"0"
	}
	"class_3_full"
	{
		"ControlName"		"Label"
		"fieldName"		"class_3_full"
		"xpos"			"252"
		"ypos"			"213" 
		"wide"			"62"
		"tall"			"20"
		"autoResize"		"0"
		"pinCorner"		"0"
		"visible"		"0"
		"enabled"		"1"
		"tabPosition"		"0"
		"labelText"		"#SDK_ClassMenu_Full"
		"textAlignment"		"center"
		"dulltext"		"0"
		"brighttext"		"1"
		"font"			"ClassMenuBodyFont"
		"wrap"			"0"
	}	
	"suicide_option"
	{
		"ControlName"		"CheckButton"
		"fieldName"		"suicide_option"
		"xpos"			"337"
		"ypos"			"374"
		"zpos"			"5"	//Tony; ontop of others, because we're putting it down overtop of the classinfo panel.
		"wide"			"200"
		"tall"			"21"
		"autoResize"		"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"tabPosition"		"0"
		"labelText"		"#SDK_ClassMenu_AutoKill"
		"textAlignment"		"east"
		"dulltext"		"0"
		"brighttext"		"0"
		"Default"		"0"		
	}
}

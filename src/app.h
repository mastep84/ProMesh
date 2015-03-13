//	created by Sebastian Reiter
//	s.b.reiter@googlemail.com
//	y10 m02 d08

#ifndef APP_H
#define APP_H

#include <QDir>
#include "main_window.h"
#include "scene/lg_object.h"
#include "common/log.h"

namespace app
{

// class CameraDesc{
// 	public:
// 		static CameraDesc&
// 		inst()	{static CameraDesc desc;
// 			 	 return desc;}

// 		ug::vector3	viewScale;

// 	private:
// 		CameraDesc() :
// 			viewScale(1, 1, 1)
// 			{}
// };



// inline CameraDesc& getCameraDesc()
// {
// 	return CameraDesc::inst();
// }

inline MainWindow* getMainWindow()
{
	static MainWindow* mainWindow = new MainWindow;
	return mainWindow;
}

inline LGObject* getActiveObject()
{
	return getMainWindow()->getActiveObject();
}

inline int getActiveSubsetIndex()
{
	return getMainWindow()->getSceneInspector()->getActiveSubsetIndex();
}

inline LGScene* getActiveScene()
{
	return getMainWindow()->get_scene();
}

inline LGObject* createEmptyLGObject(const char* name)
{
    return getMainWindow()->create_empty_object(name);
}

///	returns the path in which user-data is placed (e.g. $HOME/.promesh)
QDir UserDataDir();

///	returns the path in which user-scripts are placed (e.g. $HOME/.promesh/scripts)
QDir UserScriptDir();

///	returns the path in which temporary data may be placed (e.g. $HOME/.promesh/tmp)
QDir UserTmpDir();

///	returns the path in which the help may be placed (e.g. $HOME/.promesh/help)
QDir UserHelpDir();

void PerformClickSelection(float x, float y, bool extendSelection = false);

///	returns the version of promesh as a string
QString GetVersionString();
}//	end of namespace

#endif // APP_H

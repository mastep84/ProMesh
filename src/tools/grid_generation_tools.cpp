//	created by Sebastian Reiter
//	s.b.reiter@googlemail.com
//	y10 m05 d07

#include <vector>
#include <fstream>
#include "app.h"
#include "standard_tools.h"
#include "tools_util.h"
#include "lib_grid/algorithms/remeshing/delaunay_triangulation.h"
#include "tools/grid_generation_tools.h"

//#include "lib_discretization/spatial_discretization/disc_util/finite_volume_output.h"

using namespace std;
using namespace ug;

class ToolNewObject : public ITool
{
	public:
		void execute(LGObject* obj, QWidget* widget){
			using namespace std;
			using namespace ug;

			ToolWidget* dlg = dynamic_cast<ToolWidget*>(widget);

		//	get parameters
			QString objName = "new object";

			if(dlg){
				objName = dlg->to_string(0);
			}

		//	create a new empty object and merge the selected ones into it
			app::createEmptyLGObject(objName.toLocal8Bit().constData());
		}

		const char* get_name()		{return "New Object";}
		const char* get_tooltip()	{return "Creates a new empty object.";}
		const char* get_group()		{return "Grid Generation | Objects";}
		bool accepts_null_object_ptr()	{return true;}

		QWidget* get_dialog(QWidget* parent){
			ToolWidget *dlg = new ToolWidget(get_name(), parent, this,
											IDB_APPLY | IDB_OK | IDB_CLOSE);
		//	The name of the new object
			dlg->addTextBox(tr("new object name:"), "new object");
			return dlg;
		}
};

/**	registers a callback to automatically update internal scene-object list.*/
class ToolMergeObjects : public ITool
{
	public:
		void execute(LGObject* obj, QWidget* widget){
			using namespace std;
			using namespace ug;

			ToolWidget* dlg = dynamic_cast<ToolWidget*>(widget);

		//todo: This method makes problems, if new objects are added while
		//		the tool-dialog is opened.

		//	get parameters
			QString mergedObjName = "mergedObj";
			vector<int> selObjInds;
			bool joinSubsets = false;

			if(dlg){
				mergedObjName = dlg->to_string(0);
				selObjInds = dlg->to_index_list(1);
				joinSubsets = dlg->to_bool(2);
			}

		//	create a new empty object and merge the selected ones into it
			LGObject* mergedObj = app::createEmptyLGObject(mergedObjName.toStdString().c_str());
			Grid& mrgGrid = mergedObj->get_grid();
			SubsetHandler& mrgSH = mergedObj->get_subset_handler();

		//	The position attachment for mrgGrid
			Grid::AttachmentAccessor<VertexBase, APosition> aaPosMRG(mrgGrid, aPosition);

		//	we'll use this attachment later on on the vertices of each source grid.
			AVertexBase aVrt;

		//	iterate through all selected objects and copy their content to mergedObj
			LGScene* scene = app::getActiveScene();

			for(size_t i_obj = 0; i_obj < selObjInds.size(); ++i_obj){
				int objInd = selObjInds[i_obj];
				if(objInd >= scene->num_objects()){
					UG_LOG("Bad selection during MergeObjects. Aborting\n");
					return;
				}

				LGObject* obj = scene->get_object(objInd);
				Grid& grid = obj->get_grid();
				SubsetHandler& sh = obj->get_subset_handler();

			//	if we're joining subsets, the subsetBaseInd is always 0. If
			//	we're not joining subsets, then subsetBaseInd has to be set
			//	to the current max subset.
				int subsetBaseInd = 0;
				if(!joinSubsets)
					subsetBaseInd = mrgSH.num_subsets();

			//	we need an attachment, which tells us the associated vertex in
			//	mrgGrid for each vertex in grid.
				Grid::AttachmentAccessor<VertexBase, AVertexBase> aaVrt(grid, aVrt, true);

			//	and we need an accessor for the position attachment
				Grid::AttachmentAccessor<VertexBase, APosition> aaPos(grid, aPosition);

			//	copy vertices
				for(VertexBaseIterator iter = grid.begin<VertexBase>();
					iter != grid.end<VertexBase>(); ++iter)
				{
					VertexBase* nvrt = *mrgGrid.create_by_cloning(*iter);
					aaPosMRG[nvrt] = aaPos[*iter];
					aaVrt[*iter] = nvrt;
					mrgSH.assign_subset(nvrt, subsetBaseInd + sh.get_subset_index(*iter));
				}

			//	copy edges
				EdgeDescriptor ed;
				for(EdgeBaseIterator iter = grid.begin<EdgeBase>();
					iter != grid.end<EdgeBase>(); ++iter)
				{
					EdgeBase* eSrc = *iter;
					ed.set_vertices(aaVrt[eSrc->vertex(0)], aaVrt[eSrc->vertex(1)]);
					EdgeBase* e = *mrgGrid.create_by_cloning(eSrc, ed);
					mrgSH.assign_subset(e, subsetBaseInd + sh.get_subset_index(eSrc));
				}

			//	copy faces
				FaceDescriptor fd;
				for(FaceIterator iter = grid.begin<Face>();
					iter != grid.end<Face>(); ++iter)
				{
					Face* fSrc = *iter;
					fd.set_num_vertices(fSrc->num_vertices());
					for(size_t i = 0; i < fd.num_vertices(); ++i)
						fd.set_vertex(i, aaVrt[fSrc->vertex(i)]);

					Face* f = *mrgGrid.create_by_cloning(fSrc, fd);
					mrgSH.assign_subset(f, subsetBaseInd + sh.get_subset_index(fSrc));
				}

			//	copy volumes
				VolumeDescriptor vd;
				for(VolumeIterator iter = grid.begin<Volume>();
					iter != grid.end<Volume>(); ++iter)
				{
					Volume* vSrc = *iter;
					vd.set_num_vertices(vSrc->num_vertices());
					for(size_t i = 0; i < vd.num_vertices(); ++i)
						vd.set_vertex(i, aaVrt[vSrc->vertex(i)]);

					Volume* v = *mrgGrid.create_by_cloning(vSrc, vd);
					mrgSH.assign_subset(v, subsetBaseInd + sh.get_subset_index(vSrc));
				}

			//	remove the temporary attachment
				grid.detach_from_vertices(aVrt);

			//	now copy the names of the subset handler
			//	we overwrite old subset-infos if the name of the new one is not
			//	empty.
				for(int i_sub = 0; i_sub < sh.num_subsets(); ++i_sub){
					mrgSH.subset_info(subsetBaseInd + i_sub) = sh.subset_info(i_sub);
				}
			}

			scene->object_changed(mergedObj);
			mergedObj->geometry_changed();
			UG_LOG("The merged grid contains:\n");
			ug::PrintGridElementNumbers(mergedObj->get_grid());
		}

		const char* get_name()		{return "Merge Objects";}
		const char* get_tooltip()	{return "Merges the selected objects into a new one.";}
		const char* get_group()		{return "Grid Generation | Objects";}

		QWidget* get_dialog(QWidget* parent){
			ToolWidget *dlg = new ToolWidget(get_name(), parent, this,
											IDB_APPLY | IDB_OK | IDB_CLOSE);

		//	The name of the new object
			dlg->addTextBox(tr("new object name:"), "mergedObj");

		//	push all names of current objects
			QStringList entries;
			LGScene* scene = app::getActiveScene();

			for(int i = 0; i < scene->num_objects(); ++i){
				entries.push_back(scene->get_scene_object(i)->name());
			}
			dlg->addListBox(tr("objects:"), entries);

		//	select whether subsets should be joined
			dlg->addCheckBox(tr("join subsets:"), false);

		//	connect some signals of the scene to the refresh slot of the dialog
			connect(scene, SIGNAL(object_added(ISceneObject*)), dlg, SLOT(refreshContents()));
			connect(scene, SIGNAL(object_removed()), dlg, SLOT(refreshContents()));
			connect(scene, SIGNAL(object_properties_changed(ISceneObject*)), dlg, SLOT(refreshContents()));

			return dlg;
		}

		virtual void refresh_dialog(QWidget* dialog)
		{
			ToolWidget* dlg = dynamic_cast<ToolWidget*>(dialog);
			if(!dlg)	UG_THROW("Only pass dialogs to a tool, which were created by the tool itself!");

		//	push all names of current objects
			QStringList entries;
			LGScene* scene = app::getActiveScene();

			for(int i = 0; i < scene->num_objects(); ++i)
				entries.push_back(scene->get_scene_object(i)->name());

			dlg->setStringList(1, entries);
		}
};

class ToolCreateVertex : public ITool
{
	public:
		void execute(LGObject* obj, QWidget* widget){
			using namespace ug;
		//	since we're accepting NULL-Ptr Objects, we have to create a new one
		//	if none was supplied.
			if(!obj)
				obj = app::createEmptyLGObject("new object");

			int newSubsetIndex = 0;
			if(obj == app::getActiveObject())
				newSubsetIndex = app::getActiveSubsetIndex();

			if(newSubsetIndex < 0)
				newSubsetIndex = 0;

			CoordinatesWidget* dlg = dynamic_cast<CoordinatesWidget*>(widget);
			if(dlg){
				vector3 pos(dlg->x(), dlg->y(), dlg->z());
				promesh::CreateVertex(obj, pos, newSubsetIndex);
			}
			obj->geometry_changed();
		}

		const char* get_name()		{return "Create Vertex";}
		const char* get_tooltip()	{return "Creates a new vertex";}
		const char* get_group()		{return "Grid Generation | Basic Elements";}
		bool accepts_null_object_ptr()	{return true;}

		QWidget* get_dialog(QWidget* parent){
			return new CoordinatesWidget(get_name(), parent, this, false);
		}
};


class ToolCreateEdge : public ITool
{
	public:
		void execute(LGObject* obj, QWidget*){
		//	build an edge or a face between selected vertices.
			int newSubsetIndex = 0;
			if(obj == app::getActiveObject())
				newSubsetIndex = app::getActiveSubsetIndex();

			if(newSubsetIndex < 0)
				newSubsetIndex = 0;

			promesh::CreateEdge(obj, newSubsetIndex);

			obj->geometry_changed();
		}

		const char* get_name()		{return "Create Edge";}
		const char* get_tooltip()	{return "Creates an edge between two selected vertices.";}
		const char* get_group()		{return "Grid Generation | Basic Elements";}
};

class ToolCreateFace : public ITool
{
	public:
		void execute(LGObject* obj, QWidget*){
		//	build an edge or a face between selected vertices.
			int newSubsetIndex = 0;
			if(obj == app::getActiveObject())
				newSubsetIndex = app::getActiveSubsetIndex();

			if(newSubsetIndex < 0)
				newSubsetIndex = 0;

			promesh::CreateFace(obj, newSubsetIndex);

			obj->geometry_changed();
		}

		const char* get_name()		{return "Create Face";}
		const char* get_tooltip()	{return "Creates a face between selected vertices.";}
		const char* get_group()		{return "Grid Generation | Basic Elements";}
};


class ToolCreateVolume : public ITool
{
	public:
		void execute(LGObject* obj, QWidget*){
		//	build an edge or a face between selected vertices.
			int newSubsetIndex = 0;
			if(obj == app::getActiveObject())
				newSubsetIndex = app::getActiveSubsetIndex();

			if(newSubsetIndex < 0)
				newSubsetIndex = 0;

			promesh::CreateVolume(obj, newSubsetIndex);

			obj->geometry_changed();
		}

		const char* get_name()		{return "Create Volume";}
		const char* get_tooltip()	{return "Creates a volume between selected vertices.";}
		const char* get_group()		{return "Grid Generation | Basic Elements";}
};


class ToolCreatePlane : public ITool
{
public:
	void execute(LGObject* obj, QWidget* widget){
		ToolWidget* dlg = dynamic_cast<ToolWidget*>(widget);

	//	since we're accepting NULL-Ptr Objects, we have to create a new one
	//	if none was supplied.
		if(!obj)
			obj = app::createEmptyLGObject("new object");

		int orientation = 0;
		number width = 1.;
		number height = 1.;
		vector3 center(0, 0, 0);
		int newSI = 0;
		bool fill = false;

		if(dlg){
			orientation = dlg->to_int(0);
			width = dlg->to_double(1);
			height = dlg->to_double(2);
			center.x() = dlg->to_double(3);
			center.y() = dlg->to_double(4);
			center.z() = dlg->to_double(5);
			newSI = dlg->to_int(6);
			fill = dlg->to_bool(7);
		}

		vector3 dirWidth, dirHeight;

		switch(orientation){
			case 0:	// xy
			{
				dirWidth = vector3(0.5 * width, 0, 0);
				dirHeight = vector3(0, 0.5 * height, 0);
			}break;
			case 1:	// xz
			{
				dirWidth = vector3(0.5 * width, 0, 0);
				dirHeight = vector3(0, 0, 0.5 * height);
			}break;
			case 2:	// xy
			{
				dirWidth = vector3(0, 0.5 * width, 0);
				dirHeight = vector3(0, 0, 0.5 * height);
			}break;
		}//	end of switch

		vector3 upLeft, upRight, lowLeft, lowRight;
		VecSubtract(upLeft, center, dirWidth);
		VecAdd(upLeft, upLeft, dirHeight);

		VecSubtract(lowLeft, center, dirWidth);
		VecSubtract(lowLeft, lowLeft, dirHeight);

		VecAdd(lowRight, center, dirWidth);
		VecSubtract(lowRight, lowRight, dirHeight);

		VecAdd(upRight, center, dirWidth);
		VecAdd(upRight, upRight, dirHeight);

		promesh::CreatePlane(obj, upLeft, upRight, lowLeft, lowRight, newSI, fill);

		obj->geometry_changed();
	}

	const char* get_name()		{return "Create Plane";}
	const char* get_tooltip()	{return "Creates a plane.";}
	const char* get_group()		{return "Grid Generation | Geometries";}
	bool accepts_null_object_ptr()	{return true;}

	ToolWidget* get_dialog(QWidget* parent){
		ToolWidget *dlg = new ToolWidget(get_name(), parent, this,
								IDB_APPLY | IDB_OK | IDB_CLOSE);
		QStringList entries;
		entries.push_back(tr("xy"));
		entries.push_back(tr("xz"));
		entries.push_back(tr("yz"));

		dlg->addComboBox("orientation", entries, 0);
		dlg->addSpinBox(tr("width:"), -1.e+9, 1.e+9, 2., 1, 9);
		dlg->addSpinBox(tr("height:"), -1.e+9, 1.e+9, 2., 1, 9);
		dlg->addSpinBox(tr("center x:"), -1.e+9, 1.e+9, 0, 1, 9);
		dlg->addSpinBox(tr("center y:"), -1.e+9, 1.e+9, 0, 1, 9);
		dlg->addSpinBox(tr("center z:"), -1.e+9, 1.e+9, 0, 1, 9);
		dlg->addSpinBox(tr("subset:"), -1, 1.e+9, 0, 1, 0);
		dlg->addCheckBox(tr("fill"), false);
		return dlg;
	}
};

class ToolCreateCircle : public ITool
{
public:
	void execute(LGObject* obj, QWidget* widget){
		ToolWidget* dlg = dynamic_cast<ToolWidget*>(widget);
	//	since we're accepting NULL-Ptr Objects, we have to create a new one
	//	if none was supplied.
		if(!obj)
			obj = app::createEmptyLGObject("new object");

	//todo: add a 'regular' flag. This requires that the optimizer can
	//		be applied to a selected subset of the grid.
		number radius = 1.;
		vector3 center(0, 0, 0);
		int numRimVertices = 12;
		int newSI = 0;
		bool fill = false;

		if(dlg){
			center = dlg->to_vector3(0);
			radius = dlg->to_double(1);
			numRimVertices = dlg->to_int(2);
			newSI = dlg->to_int(3);
			fill = dlg->to_bool(4);
		}

		promesh::CreateCircle(obj, center, radius, numRimVertices, newSI, fill);

		obj->geometry_changed();
	}

	const char* get_name()		{return "Create Circle";}
	const char* get_tooltip()	{return "Creates a circle.";}
	const char* get_group()		{return "Grid Generation | Geometries";}
	bool accepts_null_object_ptr()	{return true;}

	ToolWidget* get_dialog(QWidget* parent){
		ToolWidget *dlg = new ToolWidget(get_name(), parent, this,
								IDB_APPLY | IDB_OK | IDB_CLOSE);

		dlg->addVector(tr("center:"), 3);
		dlg->addSpinBox(tr("radius:"), 0, 1.e+9, 1., 1, 9);
		dlg->addSpinBox(tr("rim vertices:"), 3, 1.e+9, 12, 1, 0);
		dlg->addSpinBox(tr("subset:"), -1, 1.e+9, 0, 1, 0);
		dlg->addCheckBox(tr("fill"), false);
		return dlg;
	}
};

class ToolCreateBox : public ITool
{
public:
	void execute(LGObject* obj, QWidget* widget){
		ToolWidget* dlg = dynamic_cast<ToolWidget*>(widget);
	//	since we're accepting NULL-Ptr Objects, we have to create a new one
	//	if none was supplied.
		if(!obj)
			obj = app::createEmptyLGObject("new object");

		vector3 boxMin(-1, -1, -1);
		vector3 boxMax(1, 1, 1);
		int newSI = 0;
		bool createVolume = false;

		if(dlg){
			boxMin.x() = dlg->to_double(0);
			boxMin.y() = dlg->to_double(1);
			boxMin.z() = dlg->to_double(2);
			boxMax.x() = dlg->to_double(3);
			boxMax.y() = dlg->to_double(4);
			boxMax.z() = dlg->to_double(5);
			newSI = dlg->to_int(6);
			createVolume = dlg->to_bool(7);
		}

		promesh::CreateBox(obj, boxMin, boxMax, newSI, createVolume);

		obj->geometry_changed();
	}

	const char* get_name()		{return "Create Box";}
	const char* get_tooltip()	{return "Creates a box.";}
	const char* get_group()		{return "Grid Generation | Geometries";}
	bool accepts_null_object_ptr()	{return true;}

	ToolWidget* get_dialog(QWidget* parent){
		ToolWidget *dlg = new ToolWidget(get_name(), parent, this,
								IDB_APPLY | IDB_OK | IDB_CLOSE);
		dlg->addSpinBox(tr("x-min:"), -1.e+9, 1.e+9, -1., 1, 9);
		dlg->addSpinBox(tr("y-min:"), -1.e+9, 1.e+9, -1., 1, 9);
		dlg->addSpinBox(tr("z-min:"), -1.e+9, 1.e+9, -1., 1, 9);
		dlg->addSpinBox(tr("x-max:"), -1.e+9, 1.e+9, 1., 1, 9);
		dlg->addSpinBox(tr("y-max:"), -1.e+9, 1.e+9, 1., 1, 9);
		dlg->addSpinBox(tr("z-max:"), -1.e+9, 1.e+9, 1., 1, 9);
		dlg->addSpinBox(tr("subset:"), -1, 1.e+9, 0, 1, 0);
		dlg->addCheckBox(tr("create volume:"), false);
		return dlg;
	}
};


class ToolCreateSphere : public ITool
{
public:
	void execute(LGObject* obj, QWidget* widget){
		ToolWidget* dlg = dynamic_cast<ToolWidget*>(widget);
	//	since we're accepting NULL-Ptr Objects, we have to create a new one
	//	if none was supplied.
		if(!obj)
			obj = app::createEmptyLGObject("new object");

	//todo: add a 'regular' flag. This requires that the optimizer can
	//		be applied to a selected subset of the grid.
		number radius = 1.;
		vector3 center(0, 0, 0);
		int numRefinements = 2;
		int newSI = 0;

		if(dlg){
			center = dlg->to_vector3(0);
			radius = dlg->to_double(1);
			numRefinements = dlg->to_int(2);
			newSI = dlg->to_int(3);
		}

		promesh::CreateSphere(obj, center, radius, numRefinements, newSI);

		obj->geometry_changed();
	}

	const char* get_name()		{return "Create Sphere";}
	const char* get_tooltip()	{return "Creates a sphere.";}
	const char* get_group()		{return "Grid Generation | Geometries";}
	bool accepts_null_object_ptr()	{return true;}

	ToolWidget* get_dialog(QWidget* parent){
		ToolWidget *dlg = new ToolWidget(get_name(), parent, this,
								IDB_APPLY | IDB_OK | IDB_CLOSE);

		dlg->addVector(tr("center:"), 3);
		dlg->addSpinBox(tr("radius:"), 0, 1.e+9, 1., 1, 9);
		dlg->addSpinBox(tr("refinements:"), 0, 1.e+9, 2, 1, 0);
		dlg->addSpinBox(tr("subset:"), -1, 1.e+9, 0, 1, 0);
		return dlg;
	}
};


class ToolCreateTetrahedron: public ITool
{
public:
	void execute(LGObject* obj, QWidget* widget){
		ToolWidget* dlg = dynamic_cast<ToolWidget*>(widget);
	//	since we're accepting NULL-Ptr Objects, we have to create a new one
	//	if none was supplied.
		if(!obj)
			obj = app::createEmptyLGObject("new object");

		int newSI = 0;
		bool createVolume = false;

		if(dlg){
			newSI = dlg->to_int(0);
			createVolume = dlg->to_bool(1);
		}

		promesh::CreateTetrahedron(obj, newSI, createVolume);

		obj->geometry_changed();
	}

	const char* get_name()		{return "Create Tetrahedron";}
	const char* get_tooltip()	{return "Creates a tetrahedron.";}
	const char* get_group()		{return "Grid Generation | Geometries";}
	bool accepts_null_object_ptr()	{return true;}

	ToolWidget* get_dialog(QWidget* parent){
		ToolWidget *dlg = new ToolWidget(get_name(), parent, this,
								IDB_APPLY | IDB_OK | IDB_CLOSE);
		dlg->addSpinBox(tr("subset:"), -1, 1.e+9, 0, 1, 0);
		dlg->addCheckBox(tr("create volume:"), false);
		return dlg;
	}
};

class ToolCreatePyramid: public ITool
{
public:
	void execute(LGObject* obj, QWidget* widget){
		ToolWidget* dlg = dynamic_cast<ToolWidget*>(widget);
	//	since we're accepting NULL-Ptr Objects, we have to create a new one
	//	if none was supplied.
		if(!obj)
			obj = app::createEmptyLGObject("new object");

		int newSI = 0;
		bool createVolume = false;

		if(dlg){
			newSI = dlg->to_int(0);
			createVolume = dlg->to_bool(1);
		}

		promesh::CreatePyramid(obj, newSI, createVolume);

		obj->geometry_changed();
	}

	const char* get_name()		{return "Create Pyramid";}
	const char* get_tooltip()	{return "Creates a pyramid.";}
	const char* get_group()		{return "Grid Generation | Geometries";}
	bool accepts_null_object_ptr()	{return true;}

	ToolWidget* get_dialog(QWidget* parent){
		ToolWidget *dlg = new ToolWidget(get_name(), parent, this,
								IDB_APPLY | IDB_OK | IDB_CLOSE);
		dlg->addSpinBox(tr("subset:"), -1, 1.e+9, 0, 1, 0);
		dlg->addCheckBox(tr("create volume:"), false);
		return dlg;
	}
};

class ToolCreatePrism: public ITool
{
public:
	void execute(LGObject* obj, QWidget* widget){
		ToolWidget* dlg = dynamic_cast<ToolWidget*>(widget);
		using namespace ug;

	//	since we're accepting NULL-Ptr Objects, we have to create a new one
	//	if none was supplied.
		if(!obj)
			obj = app::createEmptyLGObject("new object");

		int newSI = 0;
		bool createVolume = false;

		if(dlg){
			newSI = dlg->to_int(0);
			createVolume = dlg->to_bool(1);
		}

		promesh::CreatePrism(obj, newSI, createVolume);

		obj->geometry_changed();
	}

	const char* get_name()		{return "Create Prism";}
	const char* get_tooltip()	{return "Creates a prism.";}
	const char* get_group()		{return "Grid Generation | Geometries";}
	bool accepts_null_object_ptr()	{return true;}

	ToolWidget* get_dialog(QWidget* parent){
		ToolWidget *dlg = new ToolWidget(get_name(), parent, this,
								IDB_APPLY | IDB_OK | IDB_CLOSE);
		dlg->addSpinBox(tr("subset:"), -1, 1.e+9, 0, 1, 0);
		dlg->addCheckBox(tr("create volume:"), false);
		return dlg;
	}
};

/*
class ToolCreateDualGrid : public ITool
{
private:
    enum
    {
       FV1GEOMETRY = 0,
       HFV1GEOMETRY = 1
    };

public:
        void execute(LGObject* obj, QWidget* widget){
		ToolWidget* dlg = dynamic_cast<ToolWidget*>(widget);
            using namespace ug;

            int iGeom = 0;
            bool bCV, bSCV, bSCVF;

            // check dialog
            if(dlg){
                iGeom = dlg->to_int(0);
                bCV = dlg->to_bool(1);
                bSCV = dlg->to_bool(2);
                bSCVF = dlg->to_bool(3);
            }
            else
            {
                UG_LOG("No dialog\n");
                return;
            }

            // Original Grid and SubsetHandler
            Grid& grid = obj->get_grid();
            grid.enable_options(GRIDOPT_FULL_INTERCONNECTION);
            SubsetHandler& sh = obj->get_subset_handler();

            ////////////////////////////////
            // SCVF
            ////////////////////////////////
            if(bSCVF)
            {
                // Create empty dual grid
                LGObject* dualObj = app::createEmptyLGObject("DualGrid-SCVF");
                Grid& dualGrid = dualObj->get_grid();
                SubsetHandler& dualSH = dualObj->get_subset_handler();

                // Create Dual grid
                if(grid.num<Volume>() > 0)
                {
                    switch(iGeom)
                    {
                    case FV1GEOMETRY: ug::CreateGridOfSubControlVolumeFaces<FV1Geometry, APosition>(dualSH, sh, aPosition); break;
                    case HFV1GEOMETRY: ug::CreateGridOfSubControlVolumeFaces<HFV1Geometry, APosition>(dualSH, sh, aPosition); break;
                    default: UG_LOG("Geometry Type not found, although selected. Ask programmer.\n"); return;
                    }
                }
                else if (grid.num<Face>() > 0)
                {
                    // convert to 2d positions (FVGeometry depends on PositionCoordinates)
                    grid.attach_to_vertices(aPosition2);
                    dualGrid.attach_to_vertices(aPosition2);
                    ConvertMathVectorAttachmentValues<VertexBase>(grid, aPosition, aPosition2);

                    switch(iGeom)
                    {
                    case FV1GEOMETRY: ug::CreateGridOfSubControlVolumeFaces<FV1Geometry, APosition2>(dualSH, sh, aPosition2); break;
                    case HFV1GEOMETRY: ug::CreateGridOfSubControlVolumeFaces<HFV1Geometry, APosition2>(dualSH, sh, aPosition2); break;
                    default: UG_LOG("Geometry Type not found, although selected. Ask programmer.\n"); return;
                    }

                    // convert back to 3d positions (ProMesh only handles 3d)
                    ConvertMathVectorAttachmentValues<VertexBase>(dualGrid, aPosition2, aPosition);
                    grid.detach_from_vertices(aPosition2);
                    dualGrid.detach_from_vertices(aPosition2);
                }
                else if (grid.num<Edge>() > 0)
                {
                    // convert to 1d positions (FVGeometry depends on PositionCoordinates)
                    grid.attach_to_vertices(aPosition1);
                    dualGrid.attach_to_vertices(aPosition1);
                    ConvertMathVectorAttachmentValues<VertexBase>(grid, aPosition, aPosition1);

                    switch(iGeom)
                    {
                    case FV1GEOMETRY: ug::CreateGridOfSubControlVolumeFaces<FV1Geometry, APosition1>(dualSH, sh, aPosition1); break;
                    case HFV1GEOMETRY: ug::CreateGridOfSubControlVolumeFaces<HFV1Geometry, APosition1>(dualSH, sh, aPosition1); break;
                    default: UG_LOG("Geometry Type not found, although selected. Ask programmer.\n"); return;
                    }

                    // convert back to 3d positions (ProMesh only handles 3d)
                    ConvertMathVectorAttachmentValues<VertexBase>(dualGrid, aPosition1, aPosition);
                    grid.detach_from_vertices(aPosition1);
                    dualGrid.detach_from_vertices(aPosition1);
                }
                else
                {
                    UG_LOG("Grid does not contain elements, only points. Can not create Dual Grid.\n");
                    return;
                }

                // assign subset colors
                AssignSubsetColors(dualSH);

                // update view
                dualObj->geometry_changed();
            }

            ////////////////////////////////
            // SCV
            ////////////////////////////////
            if(bSCV)
            {
                // Create empty dual grid
                LGObject* dualObj = app::createEmptyLGObject("DualGrid-SCV");
                Grid& dualGrid = dualObj->get_grid();
                SubsetHandler& dualSH = dualObj->get_subset_handler();

                // Create Dual grid
                if(grid.num<Volume>() > 0)
                {
                    switch(iGeom)
                    {
                    case FV1GEOMETRY: ug::CreateGridOfSubControlVolumes<FV1Geometry, APosition>(dualSH, sh, aPosition); break;
                    case HFV1GEOMETRY: ug::CreateGridOfSubControlVolumes<HFV1Geometry, APosition>(dualSH, sh, aPosition); break;
                    default: UG_LOG("Geometry Type not found, although selected. Ask programmer.\n"); return;
                    }
                }
                else if (grid.num<Face>() > 0)
                {
                    // convert to 2d positions (FVGeometry depends on PositionCoordinates)
                    grid.attach_to_vertices(aPosition2);
                    dualGrid.attach_to_vertices(aPosition2);
                    ConvertMathVectorAttachmentValues<VertexBase>(grid, aPosition, aPosition2);

                    switch(iGeom)
                    {
                    case FV1GEOMETRY: ug::CreateGridOfSubControlVolumes<FV1Geometry, APosition2>(dualSH, sh, aPosition2); break;
                    case HFV1GEOMETRY: ug::CreateGridOfSubControlVolumes<HFV1Geometry, APosition2>(dualSH, sh, aPosition2); break;
                    default: UG_LOG("Geometry Type not found, although selected. Ask programmer.\n"); return;
                    }

                    // convert back to 3d positions (ProMesh only handles 3d)
                    ConvertMathVectorAttachmentValues<VertexBase>(dualGrid, aPosition2, aPosition);
                    grid.detach_from_vertices(aPosition2);
                    dualGrid.detach_from_vertices(aPosition2);
                }
                else if (grid.num<Edge>() > 0)
                {
                    // convert to 1d positions (FVGeometry depends on PositionCoordinates)
                    grid.attach_to_vertices(aPosition1);
                    dualGrid.attach_to_vertices(aPosition1);
                    ConvertMathVectorAttachmentValues<VertexBase>(grid, aPosition, aPosition1);

                    switch(iGeom)
                    {
                    case FV1GEOMETRY: ug::CreateGridOfSubControlVolumes<FV1Geometry, APosition1>(dualSH, sh, aPosition1); break;
                    case HFV1GEOMETRY: ug::CreateGridOfSubControlVolumes<HFV1Geometry, APosition1>(dualSH, sh, aPosition1); break;
                    default: UG_LOG("Geometry Type not found, although selected. Ask programmer.\n"); return;
                    }

                    // convert back to 3d positions (ProMesh only handles 3d)
                    ConvertMathVectorAttachmentValues<VertexBase>(dualGrid, aPosition1, aPosition);
                    grid.detach_from_vertices(aPosition1);
                    dualGrid.detach_from_vertices(aPosition1);
                }
                else
                {
                    UG_LOG("Grid does not contain elements, only points. Can not create Dual Grid.\n");
                    return;
                }

                // assign subset colors
                AssignSubsetColors(dualSH);

                // update view
                dualObj->geometry_changed();
            }

            ////////////////////////////////
            // CV
            ////////////////////////////////
            if(bCV)
            {
                // Create empty dual grid
                LGObject* dualObj = app::createEmptyLGObject("DualGrid-CV");
                Grid& dualGrid = dualObj->get_grid();
                SubsetHandler& dualSH = dualObj->get_subset_handler();

                // Create Dual grid
                if(grid.num<Volume>() > 0)
                {
                    switch(iGeom)
                    {
                    case FV1GEOMETRY: ug::CreateGridOfControlVolumes<FV1Geometry, APosition>(dualSH, sh, aPosition); break;
                    case HFV1GEOMETRY: ug::CreateGridOfControlVolumes<HFV1Geometry, APosition>(dualSH, sh, aPosition); break;
                    default: UG_LOG("Geometry Type not found, although selected. Ask programmer.\n"); return;
                    }
                }
                else if (grid.num<Face>() > 0)
                {
                    // convert to 2d positions (FVGeometry depends on PositionCoordinates)
                    grid.attach_to_vertices(aPosition2);
                    dualGrid.attach_to_vertices(aPosition2);
                    ConvertMathVectorAttachmentValues<VertexBase>(grid, aPosition, aPosition2);

                    switch(iGeom)
                    {
                    case FV1GEOMETRY: ug::CreateGridOfControlVolumes<FV1Geometry, APosition2>(dualSH, sh, aPosition2); break;
                    case HFV1GEOMETRY: ug::CreateGridOfControlVolumes<HFV1Geometry, APosition2>(dualSH, sh, aPosition2); break;
                    default: UG_LOG("Geometry Type not found, although selected. Ask programmer.\n"); return;
                    }

                    // convert back to 3d positions (ProMesh only handles 3d)
                    ConvertMathVectorAttachmentValues<VertexBase>(dualGrid, aPosition2, aPosition);
                    grid.detach_from_vertices(aPosition2);
                    dualGrid.detach_from_vertices(aPosition2);
                }
                else if (grid.num<Edge>() > 0)
                {
                    // convert to 1d positions (FVGeometry depends on PositionCoordinates)
                    grid.attach_to_vertices(aPosition1);
                    dualGrid.attach_to_vertices(aPosition1);
                    ConvertMathVectorAttachmentValues<VertexBase>(grid, aPosition, aPosition1);

                    switch(iGeom)
                    {
                    case FV1GEOMETRY: ug::CreateGridOfControlVolumes<FV1Geometry, APosition1>(dualSH, sh, aPosition1); break;
                    case HFV1GEOMETRY: ug::CreateGridOfControlVolumes<HFV1Geometry, APosition1>(dualSH, sh, aPosition1); break;
                    default: UG_LOG("Geometry Type not found, although selected. Ask programmer.\n"); return;
                    }

                    // convert back to 3d positions (ProMesh only handles 3d)
                    ConvertMathVectorAttachmentValues<VertexBase>(dualGrid, aPosition1, aPosition);
                    grid.detach_from_vertices(aPosition1);
                    dualGrid.detach_from_vertices(aPosition1);
                }
                else
                {
                    UG_LOG("Grid does not contain elements, only points. Can not create Dual Grid.\n");
                    return;
                }

                // assign subset colors
                AssignSubsetColors(dualSH);

                // update view
                dualObj->geometry_changed();
            }



        }

        const char* get_name()		{return "Create Dual Grid";}
        const char* get_tooltip()	{return "creates the dual grid consisting of"
                                                " control volumes as used in the finite volume method";}
        const char* get_group()		{return "Grid Generation";}

        ToolWidget* get_dialog(QWidget* parent){
            ToolWidget *dlg = new ToolWidget(get_name(), parent, this,
                                            IDB_OK | IDB_CANCEL);
            QStringList entries;
            entries.push_back(tr("FV1Geometry"));
            entries.push_back(tr("HFV1Geometry"));
            dlg->addComboBox("Choose Finite Volume Type", entries, 0);

            dlg->addCheckBox("Create control volumes:", true);
            dlg->addCheckBox("Create sub control volumes:", true);
            dlg->addCheckBox("Create sub control volume faces:", true);
            return dlg;
        }
 };
*/

bool CreateFractal(LGObject* obj, number scaleFac, size_t numIterations)
{
	ug::HangingNodeRefiner_Grid href(obj->get_grid());
	return ug::CreateFractal_NormalScale(obj->get_grid(), href, scaleFac, numIterations);
}

void RegisterGridGenerationTools(ToolManager* toolMgr)
{
	toolMgr->set_group_icon("Grid Generation", ":images/tool_geometry_generation.png");

	toolMgr->register_tool(new ToolNewObject);
	toolMgr->register_tool(new ToolMergeObjects);

	toolMgr->register_tool(new ToolCreateVertex);
	toolMgr->register_tool(new ToolCreateEdge);
	toolMgr->register_tool(new ToolCreateFace);
	toolMgr->register_tool(new ToolCreateVolume);

	toolMgr->register_tool(new ToolCreatePlane);
	toolMgr->register_tool(new ToolCreateCircle);
	toolMgr->register_tool(new ToolCreateBox);
	toolMgr->register_tool(new ToolCreateSphere);
	toolMgr->register_tool(new ToolCreateTetrahedron);
	toolMgr->register_tool(new ToolCreatePyramid);
	toolMgr->register_tool(new ToolCreatePrism);
    //toolMgr->register_tool(new ToolCreateDualGrid);

//	ug::bridge::Registry& reg = toolMgr->get_registry();
//	reg.add_function("TestFractal", &CreateFractal);
}

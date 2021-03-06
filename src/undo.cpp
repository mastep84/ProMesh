/*
 * Copyright (c) 2008-2015:  G-CSC, Goethe University Frankfurt
 * Copyright (c) 2006-2008:  Steinbeis Forschungszentrum (STZ Ölbronn)
 * Copyright (c) 2006-2015:  Sebastian Reiter
 * Author: Sebastian Reiter
 *
 * This file is part of ProMesh.
 * 
 * ProMesh is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License version 3 (as published by the
 * Free Software Foundation) with the following additional attribution
 * requirements (according to LGPL/GPL v3 §7):
 * 
 * (1) The following notice must be displayed in the Appropriate Legal Notices
 * of covered and combined works: "Based on ProMesh (www.promesh3d.com)".
 * 
 * (2) The following bibliography is recommended for citation and must be
 * preserved in all covered files:
 * "Reiter, S. and Wittum, G. ProMesh -- a flexible interactive meshing software
 *   for unstructured hybrid grids in 1, 2, and 3 dimensions. In preparation."
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 */

#include <string>
#include <sstream>
#include "undo.h"
#include "common/math/misc/math_util.h"
#include "common/util/file_util.h"
#include "common/util/string_util.h"

using namespace std;
using namespace ug;

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
//	UndoHistory implementation
UndoHistory::
UndoHistory() :
	m_bInitialized(false)
{
}

UndoHistory::
UndoHistory(const char* fileNamePrefix, int maxSteps) :
	m_bInitialized(true),
	m_counter(0),
	m_prefix(fileNamePrefix),
	m_maxSteps(maxSteps),
	m_numSteps(0)
{
}

bool UndoHistory::
can_undo()
{
	if(!m_bInitialized)
		return false;
	return !m_undoFiles.empty();
}

bool UndoHistory::
can_redo()
{
	if(!m_bInitialized)
		return false;
	return !m_redoFiles.empty();
}

const char* UndoHistory::
undo()
{
	if(!m_bInitialized)
		return NULL;

	if(m_undoFiles.empty())
		return NULL;

//	push the current file to the redo stack
	if(!m_currentFile.empty())
		m_redoFiles.push(m_currentFile);
	m_currentFile = m_undoFiles.back();
	m_undoFiles.pop_back();
	--m_numSteps;

	return m_currentFile.c_str();
}

const char* UndoHistory::
redo()
{
	if(!m_bInitialized)
		return NULL;

	if(m_redoFiles.empty())
		return NULL;

//	push the current file to the back of the undo files.
	if(!m_currentFile.empty())
		m_undoFiles.push_back(m_currentFile);
	m_currentFile = m_redoFiles.top();
	m_redoFiles.pop();
	++m_numSteps;

//	we don't have to check for too many undo-files here,
//	since only existing files are restored.

	return m_currentFile.c_str();
}

const char* UndoHistory::
create_history_entry()
{
	if(!m_bInitialized)
		return NULL;

//	clear the redo stack
	if(!m_redoFiles.empty()){
		while(!m_redoFiles.empty()){
			QFile rmFile(m_redoFiles.top().c_str());
			m_redoFiles.pop();
			rmFile.remove();
		}
	}

//	add undo entry
	if(!m_currentFile.empty()){
		m_undoFiles.push_back(m_currentFile);
		++m_numSteps;
	}

//	set up the new file
	stringstream ss;
	ss << m_prefix << m_counter++ << m_suffix;
	m_currentFile = ss.str();

//	check whether we have to erase a file
	if(m_numSteps == m_maxSteps){
		--m_numSteps;
		QFile rmFile(m_undoFiles.front().c_str());
		m_undoFiles.pop_front();
		rmFile.remove();
	}

	return m_currentFile.c_str();
}

void UndoHistory::
set_suffix(const char *suffix)
{
	m_suffix = suffix;
}

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
//	UndoHistoryProvider implementation
UndoHistoryProvider::
UndoHistoryProvider() :
	m_maxUndoSteps(100),
	m_historyCounter(0)
{
}

UndoHistoryProvider::
~UndoHistoryProvider()
{
	if(!m_path.empty()){
	//	remove all files in .history
		QDir history(m_parentDir);
		if(history.cd(m_historyDirName.c_str())){
			QStringList fileNames = history.entryList();
			for(QStringList::iterator iter = fileNames.begin();
				iter != fileNames.end(); ++iter)
			{
				history.remove(*iter);
			}
		}

	//	remove history itself
		m_parentDir.rmdir(m_historyDirName.c_str());
	}
}

UndoHistoryProvider& UndoHistoryProvider::
inst()
{
	static UndoHistoryProvider ufp;
	return ufp;
}

bool UndoHistoryProvider::
init(const char* path)
{
	if(m_path.empty()){
		m_path.append(path).append("/");
	//	append a unique number to the path so that each promesh instance
	//	has its own history path
		bool gotOne = false;
		for(int i = 0; i < 1000; ++i){
			m_historyDirName = ".history";
			m_historyDirName.append(ToString(urand<int>(100000, 999999)));
			string tpath = m_path;
			tpath.append(m_historyDirName);
			if(!DirectoryExists(tpath)){
				gotOne = true;
				break;
			}
		}

		if(!gotOne)
			return false;

		m_path.append(m_historyDirName);
		m_parentDir.setPath(path);
		return m_parentDir.mkdir(m_historyDirName.c_str());
	}

	return false;
}

UndoHistory UndoHistoryProvider::
create_undo_history()
{
	stringstream ss;
	ss << m_path << "/entry_" << m_historyCounter << "_";
	++m_historyCounter;
	return UndoHistory(ss.str().c_str(), m_maxUndoSteps);
}

#ifndef __IMGUI_FILE_DIALOG_H_
#define __IMGUI_FILE_DIALOG_H_

#include "imgui.h"

#include <vector>
#include <string>
#include <map>

#include <future>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <string>
#include <vector>
#include <list>

#define MAX_FILE_DIALOG_NAME_BUFFER 1024

struct FileInfoStruct
{
	char type = ' ';
	std::string filePath;
	std::string fileName;
	std::string ext;
};

class FileDialog
{
private:
	std::vector<FileInfoStruct> m_FileList;
	std::map<std::string, ImVec4> m_FilterColor;
	std::string m_SelectedFileName;
	std::string m_SelectedExt;
	std::string m_CurrentPath;
	std::vector<std::string> m_CurrentPath_Decomposition;
	std::string m_Name;
	bool m_ShowDialog;
	bool m_ShowDrives;

public:
	static char FileNameBuffer[MAX_FILE_DIALOG_NAME_BUFFER];
	static char DirectoryNameBuffer[MAX_FILE_DIALOG_NAME_BUFFER];
	static char SearchBuffer[MAX_FILE_DIALOG_NAME_BUFFER];
	static int FilterIndex;
	bool IsOk;
	bool m_AnyWindowsHovered;
	bool m_CreateDirectoryMode;

private:
	std::string dlg_key;
	std::string dlg_name;
	const char *dlg_filters;
	std::string dlg_path;
	std::string dlg_defaultFileName;
	std::string dlg_defaultExt;
	std::function<void(std::string, bool*)> dlg_optionsPane;
	size_t dlg_optionsPaneWidth;
	std::string searchTag;
	std::string dlg_userString;

public:
	static FileDialog* Instance()
	{
		static FileDialog *_instance = new FileDialog();
		return _instance;
	}

    static void RenderCurrent();
    // Open an Open File dialog for TEXT file
    static void SetCurrentOpenText();
    // Open an Open File dialog for IMAGE file
    static void SetCurrentOpenImage();
    // Open an Open File dialog for MEDIA file
    static void SetCurrentOpenMedia();

protected:

	FileDialog(); // Prevent construction
	FileDialog(const FileDialog&) {}; // Prevent construction by copying
	FileDialog& operator =(const FileDialog&) { return *this; }; // Prevent assignment
	~FileDialog(); // Prevent unwanted destruction

public:
	void OpenDialog(const std::string& vKey, const char* vName, const char* vFilters,
		const std::string& vPath, const std::string& vDefaultFileName,
		std::function<void(std::string, bool*)> vOptionsPane, size_t vOptionsPaneWidth = 250, const std::string& vUserString = "");
	void OpenDialog(const std::string& vKey, const char* vName, const char* vFilters,
		const std::string& vDefaultFileName,
		std::function<void(std::string, bool*)> vOptionsPane, size_t vOptionsPaneWidth = 250, const std::string& vUserString = "");
	void OpenDialog(const std::string& vKey, const char* vName, const char* vFilters,
		const std::string& vPath, const std::string& vDefaultFileName, const std::string& vUserString = "");
	void OpenDialog(const std::string& vKey, const char* vName, const char* vFilters,
		const std::string& vFilePathName, const std::string& vUserString = "");

	void CloseDialog(const std::string& vKey);
	bool Render(const std::string& vKey, ImVec2 geometry);
	std::string GetFilepathName();
	std::string GetCurrentPath();
	std::string GetCurrentFileName();
	std::string GetCurrentFilter();
	std::string GetUserString();

	void SetFilterColor(std::string vFilter, ImVec4 vColor);
    bool GetFilterColor(std::string vFilter, ImVec4 *vColor);
    void ClearFilterColor();

private:
	void SetPath(const std::string& vPath);
	void ScanDir(const std::string& vPath);
	void SetCurrentDir(const std::string& vPath);
	bool CreateDir(const std::string& vPath);
	void ComposeNewPath(std::vector<std::string>::iterator vIter);
	void GetDrives();
};


#endif // __IMGUI_FILE_DIALOG_H_

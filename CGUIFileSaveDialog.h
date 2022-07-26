/*This file is downloaded from irrlicht forum.
* http://irrlicht.sourceforge.net/phpBB2/viewtopic.php?t=23210&highlight=iguifileopendialog
* Code is not changed here. It is dialog for saving map files.
* It is made by MolokoTheMole - big thanks to him.
*/
#ifndef __C_GUI_FILE_SAVE_DIALOG_H_INCLUDED__
#define __C_GUI_FILE_SAVE_DIALOG_H_INCLUDED__


#include "IGUIFileOpenDialog.h"
#include "IGUIButton.h"
#include "IGUIListBox.h"
#include "IGUIEditBox.h"
#include "IFileSystem.h"

namespace irr
{
    namespace gui
    {

        class CGUIFileSaveDialog : public IGUIFileOpenDialog
        {
        public:

            //! constructor
            CGUIFileSaveDialog(const wchar_t* title,
                IGUIEnvironment* environment,
                IGUIElement* parent, s32 id,
                io::path::char_type* startDir);

            //! destructor
            virtual ~CGUIFileSaveDialog();

            //! returns the filename of the selected file. Returns NULL, if no file was selected.
            virtual const wchar_t* getFileName() const override;

            //! returns the filename of the selected file. Returns NULL, if no file was selected.
            virtual const io::path& getDirectoryName()  override;


            //! called if an event happened.
            virtual bool OnEvent(const SEvent& event) override;

            //! draws the element and its children
            virtual void draw() override;

            //! Returns the filename of the selected file. Returns NULL, if no file was selected.
      //      virtual const wchar_t* getFilename();

        protected:

            //! fills the listbox with files.
            void fillListBox();

            //! sends the event that the file has been selected.
            void sendSelectedEvent();

            //! sends the event that the file choose process has been canceld
            void sendCancelEvent();

            core::position2d<s32> DragStart;
            core::stringw FileName;
            core::stringw DirectoryName;
            bool Dragging;
            IGUIButton* CloseButton;
            IGUIButton* OKButton;
            IGUIButton* CancelButton;
            IGUIListBox* FileBox;
            IGUIElement* FileNameText;
            IGUIElement* EventParent;
            io::IFileSystem* FileSystem;
            io::path::char_type* StartDir;
            IGUIEditBox* FileEdit;

            io::IFileList* FileList;
        };


    } // end namespace gui
} // end namespace irr

#endif
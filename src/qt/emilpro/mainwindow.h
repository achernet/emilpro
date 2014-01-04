#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <qstandarditemmodel.h>

#include <model.hh>
#include <symbolfactory.hh>
#include <addresshistory.hh>

#include <unordered_map>

#include "highlighter.h"
#include "jumpdisplay-delegate.h"
#include "editinstructiondialog.h"

#include "QHexEdit/qhexedit.h"
#include "QHexEdit/qhexeditdata.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow, public emilpro::ISymbolListener
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    
    bool init(int argc, char *argv[]);


private slots:
    void on_symbolTableView_activated(const QModelIndex &index);

    void on_instructionTableView_activated(const QModelIndex &index);

    void on_referencesTableView_activated(const QModelIndex &index);

    void on_symbolTableView_entered(const QModelIndex &index);

    void on_addressHistoryListView_activated(const QModelIndex &index);

    void on_sourceTextEdit_cursorPositionChanged();

    void on_insnCurrentChanged(const QModelIndex &current, const QModelIndex &previous);

    void on_action_Forward_triggered(bool activated);

    void on_action_Backward_triggered(bool activated);

    void on_action_Mangle_names_triggered(bool activated);

    void on_action_Toggle_data_instructions_triggered(bool activated);

    void on_actionAT_T_syntax_x86_triggered(bool activated);

    void on_editInstructionPushButton_clicked();

private:
    typedef std::unordered_map<std::string, std::string> FileToStringMap_t;
    typedef std::unordered_map<int, const emilpro::IInstruction *> RowToInstruction_t;
    typedef std::unordered_map<std::string, int> AddressNameToRow_t;

    void setupSymbolView();

    void setupInstructionView();

    void setupReferencesView();

    void setupAddressHistoryView();

    void setupInstructionLabels();

    void setupInstructionEncoding();

    void setupDataView();

    void setupInfoBox();

    void addHistoryEntry(uint64_t addr);

    void refresh();

    void onSymbol(emilpro::ISymbol &sym);

	void updateInstructionView(uint64_t address, const emilpro::ISymbol &sym);

	void updateSymbolView(uint64_t address, const std::string &name = "");

	void updateInfoBox(const emilpro::IInstruction *cur);

	void updateHistoryEntry(const emilpro::AddressHistory::Entry &e);

	void updateInstructionEncoding(const emilpro::IInstruction *insn);

	void updateDataView(uint64_t address, size_t size);


    Ui::MainWindow *m_ui;
    QStandardItemModel *m_symbolViewModel;
    QStandardItemModel *m_instructionViewModel;
    QStandardItemModel *m_referencesViewModel;
    QStandardItemModel *m_addressHistoryViewModel;

    void *m_data;
    size_t m_dataSize;
    FileToStringMap_t m_sourceFileMap;
    RowToInstruction_t m_rowToInstruction;
    Highlighter *m_highlighter;
    emilpro::AddressHistory m_addressHistory;
    bool m_addressHistoryDisabled;

    AddressNameToRow_t m_addressToSymbolRowMap;

    JumpDisplayDelegate m_backwardItemDelegate;
    JumpDisplayDelegate m_forwardItemDelegate;

    EditInstructionDialog *m_editInstructionDialog;
    const emilpro::IInstruction *m_currentInstruction;

    QHexEdit *m_encodingHexEdit;
    QHexEditData *m_encodingData;

    QHexEdit *m_dataViewHexEdit;
    QHexEditData *m_dataViewData;
    uint64_t m_dataViewStart;
    uint64_t m_dataViewEnd;
    size_t m_dataViewSize;
};

#endif // MAINWINDOW_H

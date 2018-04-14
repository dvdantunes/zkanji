/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include "wordtodictionaryform.h"
#include "ui_wordtodictionaryform.h"
#include "dialogs.h"
#include "zdictionarymodel.h"
#include "words.h"
#include "wordeditorform.h"
#include "formstates.h"
#include "globalui.h"
#include "zdictionariesmodel.h"
#include "zui.h"


//-------------------------------------------------------------


WordToDictionaryForm::WordToDictionaryForm(QWidget *parent) : base(parent), ui(new Ui::WordToDictionaryForm), proxy(nullptr), dict(nullptr), dest(nullptr), addButton(nullptr), expandsize(-1)
{
    ui->setupUi(this);

    setAttribute(Qt::WA_QuitOnClose, false);
    setAttribute(Qt::WA_DeleteOnClose);

    gUI->scaleWidget(this);

    ui->wordsTable->setSelectionType(ListSelectionType::Toggle);
    ui->meaningsTable->setSelectionType(ListSelectionType::None);

    ui->wordsTable->assignStatusBar(ui->wordsStatus);
    ui->meaningsTable->assignStatusBar(ui->meaningsStatus);

    connect(gUI, &GlobalUI::dictionaryToBeRemoved, this, &WordToDictionaryForm::dictionaryToBeRemoved);
    connect(ui->buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, this, &WordToDictionaryForm::on_cancelButton_clicked);
    addButton = ui->buttonBox->addButton(tr("Create word"), QDialogButtonBox::ButtonRole::AcceptRole);
    connect(addButton, &QPushButton::clicked, this, &WordToDictionaryForm::on_addButton_clicked);
}

WordToDictionaryForm::~WordToDictionaryForm()
{
    delete ui;
}

void WordToDictionaryForm::exec(Dictionary *d, int windex)
{
    updateWindowGeometry(this);

    dict = d;
    index = windex;

    connect(dict, &Dictionary::dictionaryReset, this, &WordToDictionaryForm::close);

    // Set fixed width to the add button.

    QFontMetrics mcs = addButton->fontMetrics();
    int dif = std::abs(mcs.boundingRect(tr("Create word")).width() - mcs.boundingRect(tr("Add meanings")).width());
    addButton->setMinimumWidth(addButton->sizeHint().width() + dif);
    addButton->setMaximumWidth(addButton->minimumWidth());

    // Show the original word on top.

    DictionaryDefinitionListItemModel *model = new DictionaryDefinitionListItemModel(this);
    model->setWord(dict, windex);
    ui->wordsTable->setModel(model);

    connect(ui->wordsTable, &ZDictionaryListView::rowSelectionChanged, this, &WordToDictionaryForm::wordSelChanged);

    proxy = new DictionariesProxyModel(this);
    proxy->setDictionary(dict);
    ui->dictCBox->setModel(proxy);

    FormStates::restoreDialogSplitterState("WordToDictionary", this, ui->splitter);

    expandsize = ui->splitter->sizes().at(1);

    ui->dictCBox->setCurrentIndex(0);

    show();
}

void WordToDictionaryForm::closeEvent(QCloseEvent *e)
{
    FormStates::saveDialogSplitterState("WordToDictionary", this, ui->splitter);

    base::closeEvent(e);
}

void WordToDictionaryForm::on_addButton_clicked()
{
    // Copy member values in case the form is deleted in close.
    Dictionary *d = dict;
    int windex = index;

    std::vector<int> wdindexes;
    ui->wordsTable->selectedRows(wdindexes);
    std::sort(wdindexes.begin(), wdindexes.end());

    close();

    // Creates and opens the word editor form to add new definition to the destination word.
    editWord(d, windex, wdindexes, dest, dindex, parentWidget());
}

void WordToDictionaryForm::on_cancelButton_clicked()
{
    close();
}

void WordToDictionaryForm::on_switchButton_clicked()
{
    // Copy member values in case the form is deleted in close.
    Dictionary *d = dict;
    int windex = index;

    close();

    wordToGroupSelect(d, windex/*, parentWidget()*/);
}

void WordToDictionaryForm::on_dictCBox_currentIndexChanged(int ix)
{
    if (dest != nullptr)
        disconnect(dest, &Dictionary::dictionaryReset, this, &WordToDictionaryForm::close);
    dest = proxy->dictionaryAtRow(ix);
    if (dest != nullptr)
        connect(dest, &Dictionary::dictionaryReset, this, &WordToDictionaryForm::close);
    dindex = dest->findKanjiKanaWord(dict->wordEntry(index));

    //int h = ui->meaningsWidget->height() + ui->splitter->handleWidth(); // + ui->centralwidget->layout()->spacing();

    if (dindex == -1)
    {
        if (ui->meaningsWidget->isVisibleTo(this))
        {
            QSize s = size();
            s.setHeight(s.height() - ui->meaningsWidget->height() - ui->splitter->handleWidth());

            expandsize = ui->meaningsWidget->height();

            ui->meaningsWidget->hide();

            ui->splitter->refresh();
            ui->splitter->updateGeometry();

            updateWindowGeometry(this);

            //ui->widget->layout()->activate();
            //centralWidget()->layout()->activate();
            //layout()->activate();

            resize(s);
        }
        addButton->setText(tr("Create word"));
    }
    else
    {
        // Show existing definitions in the bottom table.
        DictionaryWordListItemModel *model = new DictionaryWordListItemModel(this);
        std::vector<int> result;
        result.push_back(dindex);
        model->setWordList(dest, std::move(result));
        if (ui->meaningsTable->model() != nullptr)
            ui->meaningsTable->model()->deleteLater();
        ui->meaningsTable->setMultiLine(true);
        ui->meaningsTable->setModel(model);

        addButton->setText(tr("Add meanings"));

        if (!ui->meaningsWidget->isVisibleTo(this))
        {
            int oldexp = expandsize;

            int siz1 = ui->widget->height();

            QSize s = size();
            s.setHeight(s.height() + ui->splitter->handleWidth() + expandsize);
            resize(s);

            //updateWindowGeometry(this);
            //centralWidget()->layout()->activate();
            //layout()->activate();

            ui->meaningsWidget->show();

            ui->splitter->setSizes({ siz1, oldexp });
        }
    }

}

void WordToDictionaryForm::on_meaningsTable_wordDoubleClicked(int windex, int dindex)
{
    // Copy member values in case the form is deleted in close.
    Dictionary *d = dest;

    close();

    // Create and open the word editor in simple edit mode to edit double clicked definition
    // of word in the destination dictionary. This method ignores the source dictionary.
    editWord(d, windex, dindex, parentWidget());
}

void WordToDictionaryForm::wordSelChanged(/*const QItemSelection &selected, const QItemSelection &deselected*/)
{
    addButton->setEnabled(ui->wordsTable->hasSelection());
}

void WordToDictionaryForm::dictionaryToBeRemoved(int index, int orderindex, Dictionary *d)
{
    if (d == dict || ZKanji::dictionaryCount() == 1)
        close();
}


//-------------------------------------------------------------


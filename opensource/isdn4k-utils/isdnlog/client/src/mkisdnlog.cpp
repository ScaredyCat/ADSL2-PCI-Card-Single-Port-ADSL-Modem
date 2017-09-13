/****************************************************************************
** KLogo meta object code from reading C++ file 'kisdnlog.h'
**
** Created: Mon May 11 01:35:26 1998
**      by: The Qt Meta Object Compiler ($Revision: 1.3 $)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#if !defined(Q_MOC_OUTPUT_REVISION)
#define Q_MOC_OUTPUT_REVISION 2
#elif Q_MOC_OUTPUT_REVISION != 2
#error Moc format conflict - please regenerate all moc files
#endif

#include "kisdnlog.h"
#include <qmetaobj.h>


const char *KLogo::className() const
{
    return "KLogo";
}

QMetaObject *KLogo::metaObj = 0;

void KLogo::initMetaObject()
{
    if ( metaObj )
	return;
    if ( strcmp(QFrame::className(), "QFrame") != 0 )
	badSuperclassWarning("KLogo","QFrame");
    if ( !QFrame::metaObject() )
	QFrame::initMetaObject();
    metaObj = new QMetaObject( "KLogo", "QFrame",
	0, 0,
	0, 0 );
}


const char *KMenu::className() const
{
    return "KMenu";
}

QMetaObject *KMenu::metaObj = 0;

void KMenu::initMetaObject()
{
    if ( metaObj )
	return;
    if ( strcmp(KMenuBar::className(), "KMenuBar") != 0 )
	badSuperclassWarning("KMenu","KMenuBar");
    if ( !KMenuBar::metaObject() )
	KMenuBar::initMetaObject();
    metaObj = new QMetaObject( "KMenu", "KMenuBar",
	0, 0,
	0, 0 );
}


const char *KHost::className() const
{
    return "KHost";
}

QMetaObject *KHost::metaObj = 0;

void KHost::initMetaObject()
{
    if ( metaObj )
	return;
    if ( strcmp(QDialog::className(), "QDialog") != 0 )
	badSuperclassWarning("KHost","QDialog");
    if ( !QDialog::metaObject() )
	QDialog::initMetaObject();
    typedef void(KHost::*m1_t0)(int);
    typedef void(KHost::*m1_t1)(int);
    typedef void(KHost::*m1_t2)();
    typedef void(KHost::*m1_t3)();
    typedef void(KHost::*m1_t4)();
    m1_t0 v1_0 = &KHost::hostChanged;
    m1_t1 v1_1 = &KHost::portChanged;
    m1_t2 v1_2 = &KHost::go;
    m1_t3 v1_3 = &KHost::Quit;
    m1_t4 v1_4 = &KHost::showHelp;
    QMetaData *slot_tbl = new QMetaData[5];
    slot_tbl[0].name = "hostChanged(int)";
    slot_tbl[1].name = "portChanged(int)";
    slot_tbl[2].name = "go()";
    slot_tbl[3].name = "Quit()";
    slot_tbl[4].name = "showHelp()";
    slot_tbl[0].ptr = *((QMember*)&v1_0);
    slot_tbl[1].ptr = *((QMember*)&v1_1);
    slot_tbl[2].ptr = *((QMember*)&v1_2);
    slot_tbl[3].ptr = *((QMember*)&v1_3);
    slot_tbl[4].ptr = *((QMember*)&v1_4);
    metaObj = new QMetaObject( "KHost", "QDialog",
	slot_tbl, 5,
	0, 0 );
}


const char *KThruput::className() const
{
    return "KThruput";
}

QMetaObject *KThruput::metaObj = 0;

void KThruput::initMetaObject()
{
    if ( metaObj )
	return;
    if ( strcmp(QFrame::className(), "QFrame") != 0 )
	badSuperclassWarning("KThruput","QFrame");
    if ( !QFrame::metaObject() )
	QFrame::initMetaObject();
    metaObj = new QMetaObject( "KThruput", "QFrame",
	0, 0,
	0, 0 );
}


const char *KChannel::className() const
{
    return "KChannel";
}

QMetaObject *KChannel::metaObj = 0;

void KChannel::initMetaObject()
{
    if ( metaObj )
	return;
    if ( strcmp(QFrame::className(), "QFrame") != 0 )
	badSuperclassWarning("KChannel","QFrame");
    if ( !QFrame::metaObject() )
	QFrame::initMetaObject();
    typedef const char*(KChannel::*m1_t0)();
    m1_t0 v1_0 = &KChannel::DeleteConnection;
    QMetaData *slot_tbl = new QMetaData[1];
    slot_tbl[0].name = "DeleteConnection()";
    slot_tbl[0].ptr = *((QMember*)&v1_0);
    metaObj = new QMetaObject( "KChannel", "QFrame",
	slot_tbl, 1,
	0, 0 );
}


const char *KSplit::className() const
{
    return "KSplit";
}

QMetaObject *KSplit::metaObj = 0;

void KSplit::initMetaObject()
{
    if ( metaObj )
	return;
    if ( strcmp(KNewPanner::className(), "KNewPanner") != 0 )
	badSuperclassWarning("KSplit","KNewPanner");
    if ( !KNewPanner::metaObject() )
	KNewPanner::initMetaObject();
    metaObj = new QMetaObject( "KSplit", "KNewPanner",
	0, 0,
	0, 0 );
}


const char *KCalls::className() const
{
    return "KCalls";
}

QMetaObject *KCalls::metaObj = 0;

void KCalls::initMetaObject()
{
    if ( metaObj )
	return;
    if ( strcmp(KTabListBox::className(), "KTabListBox") != 0 )
	badSuperclassWarning("KCalls","KTabListBox");
    if ( !KTabListBox::metaObject() )
	KTabListBox::initMetaObject();
    metaObj = new QMetaObject( "KCalls", "KTabListBox",
	0, 0,
	0, 0 );
}


const char *KCurCalls::className() const
{
    return "KCurCalls";
}

QMetaObject *KCurCalls::metaObj = 0;

void KCurCalls::initMetaObject()
{
    if ( metaObj )
	return;
    if ( strcmp(KCalls::className(), "KCalls") != 0 )
	badSuperclassWarning("KCurCalls","KCalls");
    if ( !KCalls::metaObject() )
	KCalls::initMetaObject();
    typedef bool(KCurCalls::*m1_t0)();
    m1_t0 v1_0 = &KCurCalls::ClearLines;
    QMetaData *slot_tbl = new QMetaData[1];
    slot_tbl[0].name = "ClearLines()";
    slot_tbl[0].ptr = *((QMember*)&v1_0);
    metaObj = new QMetaObject( "KCurCalls", "KCalls",
	slot_tbl, 1,
	0, 0 );
}


const char *KLog::className() const
{
    return "KLog";
}

QMetaObject *KLog::metaObj = 0;

void KLog::initMetaObject()
{
    if ( metaObj )
	return;
    if ( strcmp(QMultiLineEdit::className(), "QMultiLineEdit") != 0 )
	badSuperclassWarning("KLog","QMultiLineEdit");
    if ( !QMultiLineEdit::metaObject() )
	QMultiLineEdit::initMetaObject();
    metaObj = new QMetaObject( "KLog", "QMultiLineEdit",
	0, 0,
	0, 0 );
}


const char *KLogWin::className() const
{
    return "KLogWin";
}

QMetaObject *KLogWin::metaObj = 0;

void KLogWin::initMetaObject()
{
    if ( metaObj )
	return;
    if ( strcmp(KTopLevelWidget::className(), "KTopLevelWidget") != 0 )
	badSuperclassWarning("KLogWin","KTopLevelWidget");
    if ( !KTopLevelWidget::metaObject() )
	KTopLevelWidget::initMetaObject();
    typedef void(KLogWin::*m1_t0)();
    typedef bool(KLogWin::*m1_t1)(const char*);
    m1_t0 v1_0 = &KLogWin::Quit;
    m1_t1 v1_1 = &KLogWin::SaveToFile;
    QMetaData *slot_tbl = new QMetaData[2];
    slot_tbl[0].name = "Quit()";
    slot_tbl[1].name = "SaveToFile(const char*)";
    slot_tbl[0].ptr = *((QMember*)&v1_0);
    slot_tbl[1].ptr = *((QMember*)&v1_1);
    metaObj = new QMetaObject( "KLogWin", "KTopLevelWidget",
	slot_tbl, 2,
	0, 0 );
}


const char *KConnection::className() const
{
    return "KConnection";
}

QMetaObject *KConnection::metaObj = 0;

void KConnection::initMetaObject()
{
    if ( metaObj )
	return;
    if ( strcmp(KTopLevelWidget::className(), "KTopLevelWidget") != 0 )
	badSuperclassWarning("KConnection","KTopLevelWidget");
    if ( !KTopLevelWidget::metaObject() )
	KTopLevelWidget::initMetaObject();
    typedef bool(KConnection::*m1_t0)();
    typedef void(KConnection::*m1_t1)();
    typedef void(KConnection::*m1_t2)();
    typedef bool(KConnection::*m1_t3)();
    typedef bool(KConnection::*m1_t4)();
    typedef bool(KConnection::*m1_t5)();
    typedef bool(KConnection::*m1_t6)();
    typedef bool(KConnection::*m1_t7)();
    m1_t0 v1_0 = &KConnection::eval_message;
    m1_t1 v1_1 = &KConnection::DestroyLogWin;
    m1_t2 v1_2 = &KConnection::Quit;
    m1_t3 v1_3 = &KConnection::NewConnect;
    m1_t4 v1_4 = &KConnection::ReConnect;
    m1_t5 v1_5 = &KConnection::Disconnect;
    m1_t6 v1_6 = &KConnection::SetLogWin;
    m1_t7 v1_7 = &KConnection::SaveLogFile;
    QMetaData *slot_tbl = new QMetaData[8];
    slot_tbl[0].name = "eval_message()";
    slot_tbl[1].name = "DestroyLogWin()";
    slot_tbl[2].name = "Quit()";
    slot_tbl[3].name = "NewConnect()";
    slot_tbl[4].name = "ReConnect()";
    slot_tbl[5].name = "Disconnect()";
    slot_tbl[6].name = "SetLogWin()";
    slot_tbl[7].name = "SaveLogFile()";
    slot_tbl[0].ptr = *((QMember*)&v1_0);
    slot_tbl[1].ptr = *((QMember*)&v1_1);
    slot_tbl[2].ptr = *((QMember*)&v1_2);
    slot_tbl[3].ptr = *((QMember*)&v1_3);
    slot_tbl[4].ptr = *((QMember*)&v1_4);
    slot_tbl[5].ptr = *((QMember*)&v1_5);
    slot_tbl[6].ptr = *((QMember*)&v1_6);
    slot_tbl[7].ptr = *((QMember*)&v1_7);
    typedef void(KConnection::*m2_t0)();
    m2_t0 v2_0 = &KConnection::quit;
    QMetaData *signal_tbl = new QMetaData[1];
    signal_tbl[0].name = "quit()";
    signal_tbl[0].ptr = *((QMember*)&v2_0);
    metaObj = new QMetaObject( "KConnection", "KTopLevelWidget",
	slot_tbl, 8,
	signal_tbl, 1 );
}

// SIGNAL quit
void KConnection::quit()
{
    activate_signal( "quit()" );
}

#ifndef MYTHREAD_H
#define MYTHREAD_H

#include <QThread>
#include <QString>

class MyThread : public QThread
{
	Q_OBJECT

	public:
    		void run(void);

	public slots:
	    void esegui(QString,int,int); 

	signals:
	    void received(QString,int,int);



};
#endif

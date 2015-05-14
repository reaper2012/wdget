#ifndef MAPCHART_H
#define MAPCHART_H

#include "qframe.h"
#include <qptrlist.h>
#include <qpoint.h>

class MapChart : public QFrame
{ 
public:	

	MapChart(QWidget * parent, const char * name = 0 ,WFlags f = 0,int interval = 1000 );
	virtual void timerEvent( QTimerEvent*);
	void refresh();
	//����*sense��������
	//Ĭ�ϲ���0Ϊ��������(����pause()��ָ�����)
	void start(int* sense=0);
	void pause();
	void stop();
	//����ˮƽ�߼��,�����ڼ�����ʹ��
	bool setBlocksize(int blocksize);
	//���ùؼ�����
	bool setStep(int step);
protected:
	virtual void resizeEvent( QResizeEvent *e );
	void paintEvent( QPaintEvent *e );
	
	QPtrList<int> drawTimeList;
	QPtrList<QPoint> pointList;

	bool resizeSignal;
	bool gTimerKey;
	int* sense;

};

#endif //MAPCHART_H

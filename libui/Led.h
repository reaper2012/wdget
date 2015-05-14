#ifndef __LED_H__
#define __LED_H__

#include <qframe.h>
#include <qvaluevector.h>
#include <qscrollview.h>

class QPixmap;
class QStringList;

class TLed:public QFrame
{
public:
	enum Status{
		WAIT=0,
		CURRENT,
		DONE
	};

	struct Cell
	{
		Cell()
		{
			x=y=0;
			status=WAIT;
		}
		
		Cell(int a,int b,int c)
		{
			x=a;
			y=b;
			status=(Status)c;
		}
		
		Cell operator=(Cell c)
		{
			x=c.x;
			y=c.y;
			status=c.status;
			return *this;
		}
		int x;
		int y;
		Status status;
	};

	TLed(QWidget *parent=0,const char *name=0,WFlags f=Qt::WNoAutoErase);
	~TLed(void);

	void initLed(off_t file_total_size, off_t per_size);
	void drawLed(off_t offset, off_t length);
	void clearled();

protected:
	void paintEvent(QPaintEvent*);
	void resizeEvent(QResizeEvent*);
	void adjustSize(int w=0,int h=0);
	void loadLedImages();

private:
	//��̨���

	off_t totalSize;
	off_t blockSize;
	off_t cellCount;

	QValueVector<Cell> vvCell;
	//��ͼ���
	int cellWidth,cellHeight,cellSpacing;
	bool useImage;
	QValueVector<QPixmap> vvLedPixmap;
};


class TLedImpl : public QScrollView
{
	Q_OBJECT
public:
	
	TLedImpl(QWidget *parent=0,const char *name=0,WFlags f=0);
	~TLedImpl();

	TLed* led()
	{
		return d;
	}

public slots:
	void slotCheckScrollBarStatus();
	/**
	* ��ʼ������"����"
	* off_t file_total_size �ļ����ֽ���
	* off_t per_size ÿ��������ֽ���
	*/
	void InitLed(off_t file_total_size, off_t per_size)
	{
		d->initLed(file_total_size,per_size);
	}

	/**
	* �����ӣ����Ŀ�ʼ���������ٸ�
	* unsigned long offset �ļ�ƫ���� (byte)
	* unsigned long length �ֽڳ���
	*/
	void DrawLed(off_t offset, off_t length)
	{
		d->drawLed(offset,length);
	}

	void ClearLed()
	{
		d->clearled();
	}

protected:
	void resizeEvent(QResizeEvent*);

private:
	TLed *d;
};

#endif 

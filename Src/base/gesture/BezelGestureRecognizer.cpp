#include "BezelGesture.h"
#include "BezelGestureRecognizer.h"

#include "HostBase.h"

#include <QEvent>
#include <QTouchEvent>
#include <QTransform>
#include <QDebug>

BezelGestureRecognizer::BezelGestureRecognizer()
{
}

QGesture *BezelGestureRecognizer::create(QObject *target)
{
    return new BezelGesture;
}

QGestureRecognizer::Result BezelGestureRecognizer::recognize(QGesture *state,
                                                            QObject *,
                                                            QEvent *event)
{
	BezelGesture *q = static_cast<BezelGesture *>(state);
	const QTouchEvent *ev = static_cast<const QTouchEvent *>(event);
	const HostInfo& info = HostBase::instance()->getInfo();

	//Positions + DisplayBounds for mapping assistance
	QPoint startPos;
	QPoint pos;
	QPoint displayBounds;

	//Result to return
	QGestureRecognizer::Result result;
    
    //Switch statement to make sure we're actually handling touch events
    switch (event->type()) {
		case QEvent::TouchBegin:
		case QEvent::TouchUpdate:
		case QEvent::TouchEnd:
		{
			//Cancel if there are more than 1 fingers onscreen
			if (ev->touchPoints().size() > 1)
			{
				qCritical() << "Bezel Gesture: Canceled (Too Many Fingers)";
				return QGestureRecognizer::CancelGesture;
			}

			//COORDINATE MAPPING
			//Get the display size & screen coordinates, shift them into mappable space
			displayBounds = QPoint(info.displayWidth/2, info.displayHeight/2);
			startPos =
				QPoint(ev->touchPoints().at(0).startPos().x(),
				ev->touchPoints().at(0).startPos().y()) - displayBounds;
			pos =
				QPoint(ev->touchPoints().at(0).pos().x(),
				ev->touchPoints().at(0).pos().y()) - displayBounds;
	
			//Rotate the coordinates to our current orientation
			displayBounds = HostBase::instance()->map(displayBounds);
			startPos = HostBase::instance()->map(startPos);
			pos = HostBase::instance()->map(pos);
				
			//Make sure displayBounds is positive
			displayBounds.setX(abs(displayBounds.x()));
			displayBounds.setY(abs(displayBounds.y()));
					
			//Shift positions & display bounds into screen coordinates
			startPos += displayBounds;
			pos += displayBounds;
			displayBounds *= 2;
			
			//If the position is outside the gesture border, cancel
			if(startPos.x() > kGestureBorderSize
			&& startPos.x() < displayBounds.x() - kGestureBorderSize
			&& startPos.y() < displayBounds.y() - kGestureBorderSize)
			{
				qCritical() << "Bezel Gesture: Canceled (Outside Gesture Border)";
				return QGestureRecognizer::CancelGesture;
			}
			else
			{
				qCritical() << "Bezel Gesture: Maybe";
				result = QGestureRecognizer::MayBeGesture;
			}

			//ACTUAL RECOGNIZING
			//Work out the total distance traveled
			QPoint delta = pos - startPos;
			
			//Set the variables we already know
			q->setLastPos(q->pos());
			q->setPos(pos);
			q->setDelta(delta);
			
			//Work out the distance traveled since the last frame
			QPoint diff = q->pos() - q->lastPos();
			q->setDiff(diff);
				
			if (event->type() == QEvent::TouchUpdate) {
				//GESTURE TRIGGERING
				//Left border
				if(startPos.x() <= kGestureBorderSize)
				{
					//If the finger has moved in a horizontal direction
					if(delta.x() > 0 && delta.x() > delta.y())
					{
						//Set variables and trigger gesture
						q->setEdge(Left);
						result = QGestureRecognizer::TriggerGesture;
						qCritical() << "Bezel Gesture: Left Edge Triggered";
					}
				}
				
				//Right border
				if(startPos.x() >= displayBounds.x() - kGestureBorderSize)
				{
					//If the finger has moved in a horizontal direction
					if(delta.x() < 0 && delta.x() < delta.y())
					{
						//Set variables and trigger gesture
						q->setEdge(Right);
						result = QGestureRecognizer::TriggerGesture;
						qCritical() << "Bezel Gesture: Right Edge Triggered";
					}
				}
				
				//Bottom border
				if(startPos.y() >= displayBounds.y() - kGestureBorderSize)
				{
					//If the finger has moved in a vertical direction
					if(delta.y() < 0 && delta.y() < delta.x())
					{
						//Set variables and trigger gesture
						q->setEdge(Bottom);
						result = QGestureRecognizer::TriggerGesture;
						qCritical() << "Bezel Gesture: Bottom Edge Triggered";
					}
				}
				
				//FLICK HANDLING
				if(q->edge() == Edge(Left) || q->edge() == Edge(Right))
				{
					//If the finger's moving fast enough, set flick to 1 (right), -1 (left) or 0 (no flick)
					if(diff.x() >= 25 && diff.x() <= 100)
						q->setFlick(1);
					else if(diff.x() < 5 && diff.x() > -5)
						q->setFlick(0);
					else if(diff.x() <= -25 && diff.x() >= -100)
						q->setFlick(-1);
				}
				else if(q->edge() == Edge(Bottom))
				{
					//If the finger's moving fast enough, set flick to 1 (down), -1 (up) or 0 (no flick)
					if(diff.y() >= 25 && diff.y() <= 100)
						q->setFlick(1);
					else if(diff.y() < 5 && diff.y() > -5)
						q->setFlick(0);
					else if(diff.y() <= -25 && diff.y() >= -100)
						q->setFlick(-1);
				}	
			}
			else if (event->type() == QEvent::TouchEnd)
			{
				qCritical() << "Bezel Gesture: Finished";
				result = QGestureRecognizer::FinishGesture;
			}
			
			qCritical() << "Bezel Gesture: Flick?" << q->flick();
		}
		default: break;
    }
    return result;
}

void BezelGestureRecognizer::reset(BezelGesture *gesture)
{
	qCritical() << "Bezel Gesture: Resetting";
	gesture->setPos(QPoint(0,0));
	gesture->setLastPos(QPoint(0,0));
	gesture->setFlick(0);
	gesture->setEdge(None);
    return;
}
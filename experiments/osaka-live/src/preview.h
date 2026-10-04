#pragma once
#include "session.h"
#include <QQuickFramebufferObject>

class OsakaItem:public QQuickFramebufferObject {
    Q_OBJECT
public:
    explicit OsakaItem(QQuickItem* parent=nullptr):QQuickFramebufferObject(parent) { setMirrorVertically(true); }
    static Journey::LiveSession* session;
    Renderer* createRenderer() const override;
};

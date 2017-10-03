/*
This file is part of Telegram Desktop,
the official desktop version of Telegram messaging app, see https://telegram.org

Telegram Desktop is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

It is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

In addition, as a special exception, the copyright holders give permission
to link the code of portions of this program with the OpenSSL library.

Full license: https://github.com/telegramdesktop/tdesktop/blob/master/LICENSE
Copyright (c) 2014-2017 John Preston, https://desktop.telegram.org
*/
#include "window/window_slide_animation.h"

#include "styles/style_window.h"
#include "styles/style_boxes.h"

namespace Window {

void SlideAnimation::paintContents(Painter &p, const QRect &update) const {
	int retina = cIntRetinaFactor();

	// Animation callback can destroy "this", so we don't pass "ms".
	auto progress = _animation.current((_direction == SlideDirection::FromLeft) ? 0. : 1.);
	if (_withFade) {
		p.fillRect(update, st::boxBg);

		auto slideLeft = (_direction == SlideDirection::FromLeft);
		auto dt = slideLeft
			? (1. - progress)
			: progress;
		auto easeOut = anim::easeOutCirc(1., dt);
		auto easeIn = anim::easeInCirc(1., dt);
		auto arrivingAlpha = easeIn;
		auto departingAlpha = 1. - easeOut;
		auto leftWidthFull = _cacheUnder.width() / cIntRetinaFactor();
		auto rightWidthFull = _cacheOver.width() / cIntRetinaFactor();
		auto leftCoord = (slideLeft ? anim::interpolate(-leftWidthFull, 0, easeOut) : anim::interpolate(0, -leftWidthFull, easeIn));
		auto leftAlpha = (slideLeft ? arrivingAlpha : departingAlpha);
		auto rightCoord = (slideLeft ? anim::interpolate(0, rightWidthFull, easeIn) : anim::interpolate(rightWidthFull, 0, easeOut));
		auto rightAlpha = (slideLeft ? departingAlpha : arrivingAlpha);

		auto leftWidth = (leftWidthFull + leftCoord);
		if (leftWidth > 0) {
			p.setOpacity(leftAlpha);
			p.drawPixmap(0, 0, leftWidth, _cacheUnder.height() / retina, _cacheUnder, (_cacheUnder.width() - leftWidth * cIntRetinaFactor()), 0, leftWidth * cIntRetinaFactor(), _cacheUnder.height());
		}
		auto rightWidth = rightWidthFull - rightCoord;
		if (rightWidth > 0) {
			p.setOpacity(rightAlpha);
			p.drawPixmap(rightCoord, 0, _cacheOver, 0, 0, rightWidth * cIntRetinaFactor(), _cacheOver.height());
		}
	} else {
		auto coordUnder = anim::interpolate(0, -st::slideShift, progress);
		auto coordOver = anim::interpolate(_cacheOver.width() / cIntRetinaFactor(), 0, progress);
		if (coordOver) {
			p.drawPixmap(QRect(0, 0, coordOver, _cacheUnder.height() / retina), _cacheUnder, QRect(-coordUnder * retina, 0, coordOver * retina, _cacheUnder.height()));
			p.setOpacity(progress);
			p.fillRect(0, 0, coordOver, _cacheUnder.height() / retina, st::slideFadeOutBg);
			p.setOpacity(1);
		}
		p.drawPixmap(QRect(coordOver, 0, _cacheOver.width() / retina, _cacheOver.height() / retina), _cacheOver, QRect(0, 0, _cacheOver.width(), _cacheOver.height()));
		p.setOpacity(progress);
		st::slideShadow.fill(p, QRect(coordOver - st::slideShadow.width(), 0, st::slideShadow.width(), _cacheOver.height() / retina));
	}
	if (_topBarShadowEnabled) {
		p.setOpacity(1);
		p.fillRect(0, st::topBarHeight, _cacheOver.width() / retina, st::lineWidth, st::shadowFg);
	}
}

void SlideAnimation::setDirection(SlideDirection direction) {
	_direction = direction;
}

void SlideAnimation::setPixmaps(const QPixmap &oldContentCache, const QPixmap &newContentCache) {
	_cacheUnder = oldContentCache;
	_cacheOver = newContentCache;
}

void SlideAnimation::setTopBarShadow(bool enabled) {
	_topBarShadowEnabled = enabled;
}

void SlideAnimation::setWithFade(bool withFade) {
	_withFade = withFade;
}

void SlideAnimation::setRepaintCallback(RepaintCallback &&callback) {
	_repaintCallback = std::move(callback);
}

void SlideAnimation::setFinishedCallback(FinishedCallback &&callback) {
	_finishedCallback = std::move(callback);
}

void SlideAnimation::start() {
	auto delta = st::slideShift;
	auto fromLeft = (_direction == SlideDirection::FromLeft);
	if (fromLeft) std::swap(_cacheUnder, _cacheOver);
	_animation.start(
		[this] { animationCallback(); },
		fromLeft ? 1. : 0.,
		fromLeft ? 0. : 1.,
		st::slideDuration,
		transition());
	_repaintCallback();
}

void SlideAnimation::animationCallback() {
	_repaintCallback();
	if (!_animation.animating()) {
		if (_finishedCallback) {
			_finishedCallback();
		}
	}
}

} // namespace Window

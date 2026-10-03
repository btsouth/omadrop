export {};

const viewer = document.querySelector<HTMLDialogElement>("#image-viewer");
const image = document.querySelector<HTMLImageElement>("#viewer-image");
const title = document.querySelector<HTMLElement>("#viewer-title");
const credit = document.querySelector<HTMLElement>("#viewer-credit");
const category = document.querySelector<HTMLElement>("#viewer-category");
const count = document.querySelector<HTMLElement>("#viewer-count");
const status = document.querySelector<HTMLElement>("#viewer-status");
const media = document.querySelector<HTMLElement>(".lightbox-media");
const previous = document.querySelector<HTMLButtonElement>("#viewer-prev");
const next = document.querySelector<HTMLButtonElement>("#viewer-next");
const close = document.querySelector<HTMLButtonElement>(".lightbox-close");
const links = Array.from(document.querySelectorAll<HTMLAnchorElement>("[data-lightbox]"));

const toolbar = document.querySelector<HTMLElement>(".library-toolbar");
const search = document.querySelector<HTMLInputElement>("#library-search");
const libraryStatus = document.querySelector<HTMLElement>("#library-status");
const libraryTabs = Array.from(document.querySelectorAll<HTMLButtonElement>(".library-tabs button"));
const sections = Array.from(document.querySelectorAll<HTMLElement>("[data-library]"));
if (toolbar && search && libraryStatus && libraryTabs.length === 2 && sections.length === 2) {
  let mode = "milkdrop";
  const filter = () => {
    const query = search.value.trim().toLocaleLowerCase();
    let shown = 0;
    let total = 0;
    sections.forEach((section) => {
      section.hidden = section.dataset.library !== mode;
      section.querySelectorAll<HTMLElement>(".scene-card").forEach((card) => {
        card.hidden = !card.textContent?.toLocaleLowerCase().includes(query);
        if (!section.hidden) {
          total++;
          if (!card.hidden) shown++;
        }
      });
    });
    libraryStatus.textContent = query ? shown === 0 ? "No matches. Try another scene or artist." : `${shown} of ${total} ${mode === "milkdrop" ? "scenes" : "effects"}` : "";
  };
  const selectLibrary = (tab: HTMLButtonElement) => {
    mode = tab.id === "library-milkdrop-tab" ? "milkdrop" : "omarchy";
    libraryTabs.forEach((button) => {
      button.setAttribute("aria-selected", String(button === tab));
      button.tabIndex = button === tab ? 0 : -1;
    });
    search.placeholder = mode === "milkdrop" ? "Find a scene or artist" : "Find an effect";
    search.setAttribute("aria-label", search.placeholder);
    filter();
  };
  libraryTabs.forEach((tab) => tab.addEventListener("click", () => selectLibrary(tab)));
  toolbar.querySelector(".library-tabs")?.addEventListener("keydown", (event) => {
    const keyEvent = event as KeyboardEvent;
    if (!["ArrowLeft", "ArrowRight", "Home", "End"].includes(keyEvent.key)) return;
    keyEvent.preventDefault();
    const tab = libraryTabs[keyEvent.key === "Home" ? 0 : keyEvent.key === "End" ? 1 : mode === "milkdrop" ? 1 : 0];
    selectLibrary(tab);
    tab.focus();
  });
  sections.forEach((section, index) => {
    section.setAttribute("role", "tabpanel");
    section.setAttribute("aria-labelledby", libraryTabs[index].id);
    section.tabIndex = 0;
  });
  toolbar.hidden = false;
  search.addEventListener("input", filter);
  filter();
}

// Without dialog support or JavaScript, the links still open the original images.
if (viewer?.showModal && image && title && credit && category && count && status && media && previous && next && close) {
  let group: HTMLAnchorElement[] = [];
  let index = 0;
  let opener: HTMLAnchorElement | null = null;
  let outsidePointer: number | null = null;
  let touchStart: { x: number; y: number } | null = null;

  const showImage = (newIndex: number) => {
    index = (newIndex + group.length) % group.length;
    const link = group[index];
    title.textContent = link.dataset.title ?? "Omadrop";
    credit.textContent = link.dataset.credit ?? "";
    category.textContent = link.dataset.lightbox === "controls" ? "Made for your desktop" : link.dataset.lightbox === "omarchy" || link.dataset.lightbox === "preview" ? "Omarchy / still preview" : "MilkDrop / scene preview";
    image.alt = link.querySelector("img")?.alt ?? title.textContent;
    const thumbnail = link.querySelector("img");
    media.style.setProperty("--image-ratio", `${thumbnail?.getAttribute("width") ?? 16} / ${thumbnail?.getAttribute("height") ?? 9}`);
    image.hidden = true;
    media.setAttribute("aria-busy", "true");
    status.textContent = "Loading image…";
    image.src = link.href;
    count.textContent = `${index + 1} / ${group.length}`;
    previous.hidden = next.hidden = count.hidden = group.length < 2;
  };

  image.addEventListener("load", () => {
    image.hidden = false;
    media.setAttribute("aria-busy", "false");
    status.textContent = "";
  });
  image.addEventListener("error", () => {
    media.setAttribute("aria-busy", "false");
    status.textContent = "This image couldn’t load. Close and try again.";
  });

  links.forEach((link) => link.addEventListener("click", (event) => {
    if (event.button !== 0 || event.metaKey || event.ctrlKey || event.shiftKey || event.altKey) return;
    event.preventDefault();
    opener = link;
    group = links.filter((item) => item.dataset.lightbox === link.dataset.lightbox && !item.closest(".scene-card")?.hasAttribute("hidden"));
    showImage(group.indexOf(link));
    viewer.showModal();
    document.documentElement.classList.add("viewer-open");
    document.dispatchEvent(new CustomEvent("imageviewerchange", { detail: { open: true } }));
  }));

  close.addEventListener("click", () => viewer.close());
  previous.addEventListener("click", () => showImage(index - 1));
  next.addEventListener("click", () => showImage(index + 1));
  viewer.addEventListener("keydown", (event) => {
    if (event.key === "Tab") {
      const buttons = [close, previous, next].filter((button) => !button.hidden);
      const first = buttons[0];
      const last = buttons[buttons.length - 1];
      if (event.shiftKey && document.activeElement === first) {
        event.preventDefault();
        last.focus();
      } else if (!event.shiftKey && document.activeElement === last) {
        event.preventDefault();
        first.focus();
      }
      return;
    }
    if (group.length < 2 || !["ArrowLeft", "ArrowRight"].includes(event.key)) return;
    event.preventDefault();
    showImage(index + (event.key === "ArrowLeft" ? -1 : 1));
  });
  // Only a press and release on the overlay closes it; dragging out of the image does not.
  viewer.addEventListener("pointerdown", (event) => {
    outsidePointer = event.target === viewer ? event.pointerId : null;
  });
  viewer.addEventListener("pointerup", (event) => {
    if (event.target === viewer && event.pointerId === outsidePointer) viewer.close();
    outsidePointer = null;
  });
  viewer.addEventListener("pointercancel", () => { outsidePointer = null; });
  media.addEventListener("touchstart", (event) => {
    touchStart = event.touches.length === 1 ? { x: event.touches[0].clientX, y: event.touches[0].clientY } : null;
  }, { passive: true });
  media.addEventListener("touchend", (event) => {
    if (!touchStart || !event.changedTouches.length || group.length < 2) return;
    const dx = event.changedTouches[0].clientX - touchStart.x;
    const dy = event.changedTouches[0].clientY - touchStart.y;
    if (Math.abs(dx) > 60 && Math.abs(dx) > Math.abs(dy) * 1.5) showImage(index + (dx < 0 ? 1 : -1));
    touchStart = null;
  }, { passive: true });
  media.addEventListener("touchcancel", () => { touchStart = null; });
  viewer.addEventListener("close", () => {
    document.documentElement.classList.remove("viewer-open");
    opener?.focus({ preventScroll: true });
    outsidePointer = null;
    touchStart = null;
    document.dispatchEvent(new CustomEvent("imageviewerchange", { detail: { open: false } }));
  });
}

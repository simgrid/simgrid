
$(function() {
    /**
     * Toggle logic
     */
    $(".hidden-content").hide();
    $(".toggle-header").click(function () {
        $(this).toggleClass("open");
        $(this).next(".toggle-content").toggle('400');
    });
});

